/*!
This example shows how to use the qemu_procfs connector in conjunction
with a specific OS layer. This example does not use the `Inventory` feature of memflow
but hard-wires the connector instance with the OS layer directly.

The example is an adaption of the memflow core process list example:
https://github.com/memflow/memflow/blob/next/memflow/examples/process_list.rs

# Remarks:
The most flexible and recommended way to use memflow is to go through the inventory.
The inventory allows the user to swap out connectors and os layers at runtime.
For more information about the Inventory see the ps_inventory.rs example in this repository
or check out the documentation at:
https://docs.rs/memflow/0.1.5/memflow/connector/inventory/index.html
*/
use log::info;
use flexi_logger::{Cleanup, Criterion, Duplicate, FileSpec, Logger, Naming};

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

    // we need to actually connect to the kernel in order to get process info
    let mut kernel = memflow_hyper_rev::HypervKernelHandle::new()
        .expect("unable to initialize hyperv kernel handle");

    let process_list = kernel.process_info_list().expect("unable to read process list");

    info!(
        "{:>5} {:>10} {:>10} {:<}",
        "PID", "SYS ARCH", "PROC ARCH", "NAME"
    );

    for p in process_list {
        info!(
            "{:>5} {:^10} {:^10} {}",
            p.pid, p.sys_arch, p.proc_arch, p.name
        );
    }
}