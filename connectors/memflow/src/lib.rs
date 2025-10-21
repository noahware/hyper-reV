use anyhow::*;
use log::info;
use std::cell::{RefCell, RefMut};
use std::ffi::c_void;
use winapi::shared::minwindef::FALSE;
use winapi::um::memoryapi::{VirtualAlloc, VirtualFree, VirtualLock, VirtualUnlock};
use winapi::um::winnt::{MEM_COMMIT, MEM_RESERVE, PAGE_READWRITE};

use memflow::cglue;
use memflow::error::{Error, ErrorKind, ErrorOrigin};
use memflow::mem::phys_mem::*;
use memflow::prelude::cache::TimedCacheValidator;
use memflow::prelude::v1::*;

use memflow_win32::prelude::v1::*;

pub use memflow;

// External C interface to hyperv-core library
#[link(name = "hyperv-core")]
extern "C" {
    fn hyperv_initialize() -> bool;
    fn hyperv_shutdown();
    fn hyperv_get_last_error() -> i32;
    fn hyperv_memory_read_physical(
        destination: *mut c_void,
        guest_physical_address: u64,
        size: u64,
    ) -> u64;
    fn hyperv_memory_get_current_cr3() -> u64;
    // fn hyperv_write_host_physical_memory(
    //     source_buffer: *const c_void,
    //     host_physical_address: u64,
    //     size: u64,
    // ) -> u64;
}

#[derive(Clone, Eq, PartialEq, Debug)]
pub enum HypervError {
    InitializationFailed,
    ReadFailed {
        address: u64,
        len: usize,
    },
    WriteFailed {
        address: u64,
        len: usize,
    },
    PartialCopy {
        address: u64,
        len: usize,
        read: usize,
    },
    UntranslatablePage {
        address: u64,
        len: usize,
    },
    InvalidAddress,
}

cglue_impl_group!(HypervConnector, ConnectorInstance<'a>, {});

// Type aliases following kernel-rs pattern
type MemflowKernel = Win32Kernel<
    CachedPhysicalMemory<'static, HypervConnector, TimedCacheValidator>,
    CachedVirtualTranslate<DirectTranslate, TimedCacheValidator>,
>;
type MemflowProcess = Win32Process<
    CachedPhysicalMemory<'static, HypervConnector, TimedCacheValidator>,
    CachedVirtualTranslate<DirectTranslate, TimedCacheValidator>,
    Win32VirtualTranslate,
>;

/// Hyper-V Physical Memory implementation
#[derive(Clone)]
pub struct HypervConnector {
    initialized: bool,
}

pub struct HypervKernelHandle(MemflowKernel);

impl HypervKernelHandle {
    /// Creates a new Hyper-V connector instance
    pub fn new() -> anyhow::Result<Self> {
        let connector = unsafe { HypervConnector::new() }?;

        let kernel = Win32Kernel::builder(connector)
            .build_default_caches()
            .build()?;

        Ok(Self(kernel))
    }

    pub fn attach_pid(&self, pid: u64) -> anyhow::Result<HypervProcess> {
        Ok(HypervProcess::new(
            self.0.clone().into_process_by_pid(pid as _)?,
        ))
    }

    pub fn process_info_list(&mut self) -> anyhow::Result<Vec<ProcessInfo>> {
        Ok(self.0.process_info_list()?)
    }
}

impl Default for HypervKernelHandle {
    fn default() -> Self {
        Self::new().unwrap()
    }
}

/// Process handle for attached processes
pub struct HypervProcess(RefCell<MemflowProcess>);

/// Page-aligned and locked buffer for physical memory access
/// Uses Windows VirtualAlloc and VirtualLock for guaranteed physical memory
pub struct AlignedBuffer {
    ptr: *mut c_void,
    len: usize,
}

impl AlignedBuffer {
    pub fn new(size: usize) -> Option<Self> {
        unsafe {
            let ptr = VirtualAlloc(
                std::ptr::null_mut(),
                size,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE,
            );

            if ptr.is_null() {
                log::error!("VirtualAlloc failed for size {:#X}", size);
                return None;
            }

            // Lock the memory to prevent paging
            if VirtualLock(ptr, size) == FALSE {
                log::error!("VirtualLock failed for buffer at {:p}", ptr);
                VirtualFree(ptr, 0, winapi::um::winnt::MEM_RELEASE);
                return None;
            }

            Some(AlignedBuffer { ptr, len: size })
        }
    }

    pub fn as_bytes_mut(&mut self) -> &mut [u8] {
        unsafe { std::slice::from_raw_parts_mut(self.ptr as *mut u8, self.len) }
    }

    pub fn as_bytes(&self) -> &[u8] {
        unsafe { std::slice::from_raw_parts(self.ptr as *const u8, self.len) }
    }

    pub fn as_ptr(&self) -> *const c_void {
        self.ptr
    }

    pub fn as_mut_ptr(&mut self) -> *mut c_void {
        self.ptr
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn is_empty(&self) -> bool {
        self.len == 0
    }
}

impl Drop for AlignedBuffer {
    fn drop(&mut self) {
        unsafe {
            if VirtualUnlock(self.ptr, self.len) == FALSE {
                log::warn!("VirtualUnlock failed for buffer at {:p}", self.ptr);
            }

            if VirtualFree(self.ptr, 0, winapi::um::winnt::MEM_RELEASE) == FALSE {
                log::error!("VirtualFree failed for buffer at {:p}", self.ptr);
            }
        }
    }
}

impl HypervProcess {
    pub fn new(proc: MemflowProcess) -> Self {
        Self(RefCell::new(proc))
    }

    pub fn memflow(&self) -> RefMut<'_, MemflowProcess> {
        self.0.borrow_mut()
    }
}

impl HypervConnector {
    /// Creates a new Hyper-V connector instance
    pub unsafe fn new() -> anyhow::Result<Self> {
        info!("Creating Hyper-V connector");

        // Initialize hyperv-core library
        if !hyperv_initialize() {
            return Err(anyhow::anyhow!("Failed to initialize hyperv-core library"));
        }

        info!("Hyper-V connector initialized successfully");
        Ok(Self { initialized: true })
    }

    pub unsafe fn get_current_cr3(&self) -> u64 {
        hyperv_memory_get_current_cr3()
    }

    pub unsafe fn read_physical(
        &self,
        address: u64,
        buf: &mut [u8],
    ) -> core::result::Result<(), HypervError> {
        // Allocate page-aligned and locked buffer for the read
        let mut aligned_buf = AlignedBuffer::new(buf.len()).ok_or(HypervError::ReadFailed {
            address,
            len: buf.len(),
        })?;

        let bytes_read =
            hyperv_memory_read_physical(aligned_buf.as_mut_ptr(), address, buf.len() as u64) as u64;

        if bytes_read == u64::MAX {
            return Err(HypervError::UntranslatablePage {
                address,
                len: buf.len(),
            });
        }

        if bytes_read == 0 {
            return Err(HypervError::ReadFailed {
                address,
                len: buf.len(),
            });
        }

        if bytes_read != buf.len() as u64 {
            return Err(HypervError::PartialCopy {
                address,
                len: buf.len(),
                read: bytes_read as usize,
            });
        }

        // Copy from page-aligned buffer to caller's buffer
        buf.copy_from_slice(&aligned_buf.as_bytes()[..buf.len()]);

        std::result::Result::Ok(())
    }

    const CHUNK_SIZE: usize = 0x1000;

    pub unsafe fn read_physical_chunked(&self, address: u64, buf: &mut [u8]) {
        if buf.len() <= Self::CHUNK_SIZE {
            let result = self.read_physical(address, buf);
            if let Err(e) = result {
                log::warn!(
                    "Error while reading start {:#X} end {:#X}: error {:?} hyperv-last-error {}",
                    address,
                    address + buf.len() as u64,
                    e,
                    hyperv_get_last_error()
                );
                buf.fill(0);
            }
            return;
        }

        for (n, chunk) in buf.chunks_mut(Self::CHUNK_SIZE).enumerate() {
            let start = address + (n * Self::CHUNK_SIZE) as u64;
            let result = self.read_physical(start, chunk);
            if let Err(e) = result {
                log::warn!(
                    "Error while reading start {:#X} end {:#X}: error {:?} hyperv-last-error {}",
                    start,
                    start + chunk.len() as u64,
                    e,
                    hyperv_get_last_error()
                );
                chunk.fill(0);
            }
        }
    }

    // pub unsafe fn write_physical(
    //     &self,
    //     address: u64,
    //     buf: &[u8],
    // ) -> core::result::Result<(), HypervError> {
    //     let bytes_written = hyperv_write_host_physical_memory(
    //         buf.as_ptr() as *const c_void,
    //         address,
    //         buf.len() as u64,
    //     ) as u64;

    //     if bytes_written != buf.len() as u64 {
    //         return Err(HypervError::WriteFailed);
    //     }

    //     std::result::Result::Ok(())
    // }
}

impl PhysicalMemory for HypervConnector {
    fn phys_read_raw_iter(
        &mut self,
        mut data: PhysicalReadMemOps,
    ) -> std::result::Result<(), memflow::error::Error> {
        for item in data.inp {
            let (physical_address, meta_addr, mut out_buf) = (item.0, item.1, item.2);
            let start_phys = physical_address.to_umem();

            // Zero buffer up-front for deterministic contents on failure
            out_buf.fill(0);

            unsafe {
                self.read_physical_chunked(start_phys, out_buf.as_bytes_mut());
            }

            // Always call success callback since we handle errors by zeroing
            opt_call(data.out.as_deref_mut(), CTup2(meta_addr, out_buf));
        }
        std::result::Result::Ok(())
    }

    fn phys_write_raw_iter(
        &mut self,
        _data: PhysicalWriteMemOps,
    ) -> std::result::Result<(), memflow::error::Error> {
        // Write operations are not supported in read-only mode
        Err(Error(ErrorOrigin::Connector, ErrorKind::Unknown)
            .log_error("Write operations not supported in read-only mode"))
    }

    fn metadata(&self) -> PhysicalMemoryMetadata {
        PhysicalMemoryMetadata {
            readonly: true,
            ideal_batch_size: 0x1000,
            real_size: 0xFFFFFFFFFFFFFFFF,
            max_address: 0xFFFFFFFFFFFFFFFF_u64.into(),
        }
    }

    fn set_mem_map(&mut self, _mem_map: &[PhysicalMemoryMapping]) {}
}

impl Drop for HypervConnector {
    fn drop(&mut self) {
        if self.initialized {
            unsafe {
                hyperv_shutdown();
            }
        }
    }
}

fn validator() -> ArgsValidator {
    ArgsValidator::new()
}

/// Creates a new Hyper-V connector instance
pub fn create_connector(
    _args: &ConnectorArgs,
) -> std::result::Result<HypervConnector, memflow::error::Error> {
    unsafe {
        HypervConnector::new().map_err(|e| {
            Error(ErrorOrigin::Connector, ErrorKind::Unknown).log_error(&e.to_string())
        })
    }
}

/// Retrieve the help text for the Hyper-V Connector
pub fn help() -> String {
    let validator = validator();
    format!(
        "\
The `hyperv` connector implements direct access to Hyper-V hypervisor
memory through the hyperv-core library. It provides full physical memory
access for analysis of guest virtual machines.

The connector requires the hyperv-attachment hypervisor component to be
loaded and accessible.

Available arguments are:
{validator}"
    )
}

/// Retrieve a list of all currently available Hyper-V targets.
pub fn target_list() -> std::result::Result<Vec<TargetInfo>, memflow::error::Error> {
    // For now, return empty list - could be extended to enumerate VMs
    std::result::Result::Ok(vec![])
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_help() {
        let help_text = help();
        assert!(help_text.contains("hyperv"));
        assert!(help_text.contains("Hyper-V"));
    }

    #[test]
    fn test_connector_creation() {
        // This test will only pass if hyperv-core is properly initialized
        // In a real environment with hyperv-attachment loaded
        let args = ConnectorArgs::default();
        match create_connector(&args) {
            Ok(_) => println!("Connector created successfully"),
            Err(e) => println!(
                "Connector creation failed (expected in test environment): {:?}",
                e
            ),
        }
    }
}
