/*!
This example shows how to use the hyperv connector to read physical memory
from a Hyper-V guest machine. It also evaluates the number of read cycles per second
and prints them to stdout.
*/

use std::time::Instant;

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

    let connector_args = ConnectorArgs::default();
    let mut connector = memflow_hyper_rev::create_connector(&connector_args)
        .expect("unable to initialize hyperv connector");

    let metadata = connector.metadata();
    info!("Received metadata: {:?}", metadata);

    // Test reading from a common physical memory location
    let mut mem = vec![0; 8];
    connector
        .phys_view()
        .read_raw_into(Address::from(0x1000), &mut mem)
        .expect("unable to read physical memory");
    info!("Received memory at 0x1000: {:?}", mem);

    // Performance test
    info!("Starting performance test...");
    let start = Instant::now();
    let mut counter = 0;
    let test_duration_secs = 10;
    
    loop {
        let mut buf = vec![0; 0x1000];
        if let Ok(_) = connector
            .phys_view()
            .read_raw_into(Address::from(0x1000 + (counter % 100) * 0x1000), &mut buf)
        {
            counter += 1;
        }

        if (counter % 1000) == 0 {
            let elapsed = start.elapsed().as_secs();
            if elapsed >= test_duration_secs {
                break;
            }
            
            let elapsed_ms = start.elapsed().as_millis() as f64;
            if elapsed_ms > 0.0 {
                info!("{:.2} reads/sec", (f64::from(counter)) / elapsed_ms * 1000.0);
                info!("{:.4} ms/read", elapsed_ms / (f64::from(counter)));
            }
        }
    }

    let final_elapsed = start.elapsed().as_millis() as f64;
    info!("Performance test completed:");
    info!("  Total reads: {}", counter);
    info!("  Total time: {:.2} seconds", final_elapsed / 1000.0);
    info!("  Average: {:.2} reads/sec", (f64::from(counter)) / final_elapsed * 1000.0);
}