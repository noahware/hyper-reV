/*!
This example shows how to use the hyperv connector to scan physical memory
for process structures and enumerate them without OS layer assistance.
*/

use log::info;
use flexi_logger::{Cleanup, Criterion, Duplicate, FileSpec, Logger, Naming};

use memflow::prelude::v1::*;

fn init_logger() {
    // Write logs to stdout and rotate files under ./logs
    Logger::try_with_env_or_str("info")
        .unwrap()
        .duplicate_to_stdout(Duplicate::All)
        .log_to_file(FileSpec::default().directory("logs"))
        .rotate(Criterion::Size(10_000_000), Naming::Timestamps, Cleanup::KeepLogFiles(7))
        .start()
        .unwrap();
}

fn main() {
    init_logger();

    // Create hyperv connector
    let connector_args = ConnectorArgs::default();
    let mut connector = memflow_hyper_rev::create_connector(&connector_args)
        .expect("unable to initialize hyperv connector");

    info!("Hyper-V connector initialized successfully");

    let metadata = connector.metadata();
    info!("Physical memory metadata: {:?}", metadata);

    // Scan for common Windows structures by looking for patterns
    info!("Scanning physical memory for Windows signatures...");

    let mut found_signatures = 0;
    let scan_size = 0x1000; // 4KB chunks
    let max_scan = 0x10000000; // Scan first 256MB

    for addr in (0x1000..max_scan).step_by(scan_size) {
        let mut buffer = vec![0u8; scan_size];
        
        if connector
            .phys_view()
            .read_raw_into(Address::from(addr), &mut buffer)
            .is_ok()
        {
            // Look for common Windows signatures
            if let Some(offset) = find_pattern(&buffer, b"KDBG") {
                info!("Found potential KDBG signature at physical address: 0x{:x}", addr + offset as u64);
                found_signatures += 1;
            }

            if let Some(offset) = find_pattern(&buffer, b"This program cannot be run in DOS mode") {
                info!("Found PE DOS stub at physical address: 0x{:x}", addr + offset as u64);
                found_signatures += 1;
            }

            if let Some(offset) = find_pattern(&buffer, b"RSDS") {
                info!("Found PDB signature at physical address: 0x{:x}", addr + offset as u64);
                found_signatures += 1;
            }

            // Look for potential page table structures (x64 Windows)
            // for i in (0..buffer.len() - 8).step_by(8) {
            //     let qword = u64::from_le_bytes([
            //         buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3],
            //         buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7]
            //     ]);

            //     // Check if it looks like a valid page table entry (present bit set, valid physical address range)
            //     if (qword & 1) == 1 && (qword & 0x000FFFFFFFFFF000) < metadata.real_size {
            //         let pte_addr = addr + i as u64;
            //         if pte_addr % 0x1000 == 0 { // Only log page-aligned potential PTEs
            //             info!("Potential page table entry at 0x{:x}: 0x{:016x}", pte_addr, qword);
            //             found_signatures += 1;
                        
            //             if found_signatures >= 999999 { // Limit output
            //                 break;
            //             }
            //         }
            //     }
            // }
        }

        if found_signatures >= 999999 {
            break;
        }

        // Progress indicator
        if addr % 0x1000000 == 0 {
            info!("Scanned up to 0x{:x} ({} signatures found)", addr, found_signatures);
        }
    }

    info!("Memory scan completed. Found {} potential signatures.", found_signatures);

    // Test memory write capability (write to a safe location)
    info!("Skipping memory write test: connector is read-only");
}

fn find_pattern(haystack: &[u8], needle: &[u8]) -> Option<usize> {
    haystack
        .windows(needle.len())
        .position(|window| window == needle)
}