# memflow-hyperv

A [MemFlow](https://github.com/memflow/memflow) connector for Hyper-V hypervisor using the hyperv-core library.

This connector provides direct physical memory access to Hyper-V guest virtual machines through the hypervisor layer, enabling powerful memory analysis and forensics capabilities.

## Features

- **Direct Hypervisor Access**: Bypass guest OS protections by accessing memory directly through the hypervisor
- **High Performance**: Optimized memory operations with scatter/gather support
- **Full Memory Access**: Read and write capabilities for complete physical memory space
- **MemFlow Integration**: Standard MemFlow connector interface for compatibility with existing tools

## Requirements

- Windows x64 host system
- Hyper-V role enabled
- `hyperv-attachment` hypervisor component loaded
- `hyperv-core` library compiled and available

## Installation

### Building from Source

```bash
# Clone the repository
git clone https://github.com/your-repo/memflow-hyperv
cd memflow-hyperv

# Build the connector
cargo build --release

# Install as MemFlow plugin
mkdir -p ~/.local/lib/memflow/
cp target/release/libmemflow_hyperv.so ~/.local/lib/memflow/  # Linux
# or
copy target\release\memflow_hyperv.dll %USERPROFILE%\.memflow\  # Windows
```

## Usage

### Basic Memory Reading

```rust
use memflow::prelude::v1::*;

fn main() -> Result<()> {
    // Create hyperv connector
    let mut connector = memflow::connector("hyperv", &ConnectorArgs::new())?;
    
    // Read physical memory
    let mut buffer = vec![0u8; 0x1000];
    connector.phys_read_into(Address::from(0x1000), &mut buffer)?;
    
    println!("Read {} bytes from physical address 0x1000", buffer.len());
    Ok(())
}
```

### Process Analysis with Win32 OS Layer

```rust
use memflow::prelude::v1::*;

fn main() -> Result<()> {
    // Create connector and OS instance
    let connector = memflow::connector("hyperv", &ConnectorArgs::new())?;
    let mut os = memflow::os("win32", connector, &OsArgs::new())?;
    
    // List processes
    let processes = os.process_info_list()?;
    for proc in processes.iter().take(10) {
        println!("PID: {} | Name: {}", proc.pid, proc.name);
    }
    
    Ok(())
}
```

### Command Line Usage

```bash
# Use with memflow-cli
memflow -c hyperv -t win32 ps

# Use with memflowup  
memflowup -c hyperv

# Use with other memflow tools
memflow-repl -c hyperv -t win32
```

## Examples

The `examples/` directory contains several demonstration programs:

- `read_phys.rs` - Basic physical memory reading and performance testing
- `ps_win32.rs` - Process enumeration using Win32 OS layer
- `scan_memory.rs` - Low-level memory scanning for structures

Run examples with:

```bash
cargo run --example read_phys
cargo run --example ps_win32
cargo run --example scan_memory
```

## Configuration

The hyperv connector accepts the following arguments:

- No specific arguments required - the connector auto-detects the hypervisor

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────────┐
│   MemFlow       │    │  memflow-hyperv  │    │   hyperv-core       │
│   Framework     │◄──►│   Connector      │◄──►│   Library           │
│                 │    │                  │    │                     │
└─────────────────┘    └──────────────────┘    └─────────────────────┘
                                                          │
                                                          ▼
                                                ┌─────────────────────┐
                                                │  hyperv-attachment  │
                                                │   Hypervisor        │
                                                │   Component         │
                                                └─────────────────────┘
                                                          │
                                                          ▼
                                                ┌─────────────────────┐
                                                │    Hyper-V Guest    │
                                                │   Virtual Machine   │
                                                └─────────────────────┘
```

## Comparison with Other Connectors

| Connector | Access Method | OS Support | Performance | Use Cases |
|-----------|---------------|------------|-------------|-----------|
| `hyperv` | Hypervisor | Windows Guest | High | Live analysis, forensics |
| `vmware` | VMware APIs | Windows/Linux | Medium | Development, testing |
| `coredump` | File-based | Any | Highest | Post-mortem analysis |
| `pcileech` | DMA | Any | High | Hardware-based analysis |

## Troubleshooting

### "Failed to initialize hyperv-core library"

- Ensure `hyperv-attachment` is properly loaded in the hypervisor
- Verify you have administrator privileges
- Check that Hyper-V is enabled and running

### "Unable to read physical memory"

- Verify guest VM is running and accessible
- Check memory address ranges are valid
- Ensure no memory protection mechanisms are interfering

### Performance Issues

- Use larger buffer sizes for bulk operations
- Consider memory locality when accessing data
- Monitor system resources during analysis

## Development

### Building

```bash
# Debug build
cargo build

# Release build with optimizations
cargo build --release

# Run tests
cargo test

# Run with logging
RUST_LOG=debug cargo run --example read_phys
```

### Testing

The connector can be tested against running Hyper-V VMs. Ensure you have:

1. A Windows VM running in Hyper-V
2. `hyperv-attachment` component loaded
3. Appropriate test permissions

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality  
4. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [MemFlow Framework](https://github.com/memflow/memflow) for the excellent memory analysis framework
- [memflow-coredump](https://github.com/memflow/memflow-coredump) for connector architecture reference
- The hyperv-core library developers