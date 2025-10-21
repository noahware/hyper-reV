# Hyperv-Core Connectors

This directory contains connectors to integrate hyperv-core with popular memory analysis frameworks.

## MemFlow Connector (Rust)

### Installation
```bash
cd connectors/memflow
cargo build --release
```

### Usage with MemFlow
```rust
use memflow::prelude::v1::*;

// Create connector
let mut connector = memflow::connector("hyperv", &ConnectorArgs::new())?;

// Read physical memory  
let mut buf = vec![0u8; 0x1000];
connector.phys_read_into(Address::from(0x1000), &mut buf)?;

// Write physical memory
let data = vec![0x90u8; 16]; // NOP sled
connector.phys_write(Address::from(0x2000), &data)?;
```

## LeechCore Connector (C/C++)

### Building
1. Ensure LeechCore SDK is available
2. Set `LEECHCORE_PATH` environment variable
3. Build the solution including the leechcore-hyperv project

### Usage with MemProcFS
```bash
# Copy leechcore_hyperv.dll to LeechCore plugins directory
copy build\x64\Release\leechcore_hyperv.dll %LEECHCORE_PATH%\plugins\

# Use with MemProcFS
memprocfs.exe -device hyperv -v -forensic 1
```

### Usage with LeechCore directly
```c
#include <leechcore.h>

// Initialize
LC_CONFIG cfg = { .szDevice = "hyperv" };
HANDLE hLC = LcCreate(&cfg);

// Read memory
BYTE buffer[0x1000];
LcRead(hLC, 0x1000, sizeof(buffer), buffer);

// Write memory  
BYTE data[] = { 0x90, 0x90, 0x90, 0x90 }; // NOPs
LcWrite(hLC, 0x2000, sizeof(data), data);

// Cleanup
LcClose(hLC);
```

## VMware Integration Example

For comparison with VMware connectors:

### MemFlow VMware-like usage
```rust
// Instead of vmware connector
let connector = memflow::connector("hyperv", &ConnectorArgs::new())?;

// Same API as other connectors
let os = memflow::os("win32", connector, &OsArgs::new())?;
let process = os.process_from_name("notepad.exe")?;
```

### MemProcFS Hyper-V usage  
```bash
# Instead of VMware
memprocfs.exe -device vmware

# Use Hyper-V
memprocfs.exe -device hyperv
```

## Benefits

1. **Standard Interfaces**: Use existing tools and workflows
2. **Cross-Platform**: MemFlow works on Linux, Windows, macOS  
3. **Rich Ecosystem**: Access to existing plugins and tools
4. **Performance**: Direct hypervisor access without VM tools

## Requirements

### MemFlow
- Rust toolchain
- MemFlow SDK
- hyperv-core library

### LeechCore/MemProcFS
- LeechCore SDK
- Visual Studio 2019+
- hyperv-core library
- Windows x64

## Troubleshooting

### MemFlow Issues
- Ensure hyperv-core library is in PATH or LD_LIBRARY_PATH
- Check Rust target architecture matches library

### LeechCore Issues  
- Verify LeechCore plugin directory permissions
- Check DLL dependencies with Dependency Walker
- Ensure hypervisor is running and accessible