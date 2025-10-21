/*!
Use this to test the performance of the memflow hyperv connector by performing repeated hypercals.
*/

use log::info;
use memflow::prelude::v1::*;
use std::time::Instant;

fn main() {
    env_logger::init_from_env(env_logger::Env::new().default_filter_or("info"));

    let connector_args = ConnectorArgs::default();

    let connector = memflow_hyper_rev::create_connector(&connector_args)
        .expect("unable to initialize hyperv connector");

    // Performance test
    info!("Starting performance test...");
    let start = Instant::now();
    let counter = 0;
    let test_duration_secs = 10;

    loop {
        // Call get_current_cr3 50,000 times per frame
        let mut last_cr3 = 0u64;
        for _ in 0..50_000 {
            // Safety: get_current_cr3 is marked unsafe
            last_cr3 = unsafe { connector.get_current_cr3() };
        }
        info!("Last CR3 read in this frame: {:#x}", last_cr3);

        if (counter % 1000) == 0 {
            let elapsed = start.elapsed().as_secs();
            if elapsed >= test_duration_secs {
                break;
            }

            let elapsed_ms = start.elapsed().as_millis() as f64;
            if elapsed_ms > 0.0 {
                info!(
                    "{:.2} reads/sec",
                    (f64::from(counter)) / elapsed_ms * 1000.0
                );
                info!("{:.4} ms/read", elapsed_ms / (f64::from(counter)));
            }
        }
    }

    let final_elapsed = start.elapsed().as_millis() as f64;
    info!("Performance test completed:");
    info!("  Total reads: {}", counter);
    info!("  Total time: {:.2} seconds", final_elapsed / 1000.0);
    info!(
        "  Average: {:.2} reads/sec",
        (f64::from(counter)) / final_elapsed * 1000.0
    );
}
