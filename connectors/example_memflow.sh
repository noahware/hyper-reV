#!/bin/bash

# Example: Using hyperv-core with MemFlow on Linux

# 1. Build the MemFlow connector
echo "Building MemFlow connector..."
cd connectors/memflow
cargo build --release

# 2. Install the plugin
echo "Installing MemFlow plugin..."
mkdir -p ~/.local/lib/memflow/
cp target/release/libmemflow_hyperv.so ~/.local/lib/memflow/

# 3. Example Python script using MemFlow
cat > example_usage.py << 'EOF'
#!/usr/bin/env python3

import memflow

def main():
    # Connect using hyperv connector
    print("Connecting to Hyper-V...")
    connector = memflow.ConnectorInventory().create_connector("hyperv")
    
    if not connector:
        print("Failed to create hyperv connector")
        return
    
    print("Connected successfully!")
    
    # Read physical memory
    print("Reading physical memory...")
    data = connector.phys_read(0x1000, 0x100)
    print(f"Read {len(data)} bytes from physical address 0x1000")
    print("First 32 bytes:", ' '.join(f'{b:02x}' for b in data[:32]))
    
    # Create OS instance for process analysis
    print("Creating OS instance...")
    os = memflow.OsInventory().create_os("win32", connector)
    
    if os:
        print("OS instance created successfully!")
        
        # List processes
        processes = os.process_list()
        print(f"Found {len(processes)} processes:")
        
        for proc in processes[:10]:  # Show first 10
            print(f"  PID: {proc.pid:5} | Name: {proc.name}")
    
    print("Done!")

if __name__ == "__main__":
    main()
EOF

chmod +x example_usage.py

echo "Example created: example_usage.py"
echo "Run with: python3 example_usage.py"