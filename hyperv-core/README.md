# Hyperv-Core Library

A static library for Hyper-V hypervisor interactions providing core functionality for memory operations and kernel hooking.

## Features

- **Memory Operations**: Read/write physical and virtual memory
- **Address Translation**: Virtual to physical address translation  
- **Kernel Hooking**: SLAT-based kernel hook system
- **Error Handling**: Comprehensive error reporting
- **Clean API**: Simple and intuitive interface

## API Reference

### Core Functions

```cpp
#include <hyperv/core.h>

// Initialize the library (must be called first)
bool hyperv::initialize();

// Shutdown and cleanup
void hyperv::shutdown();

// Get last error
hyperv::error_code hyperv::get_last_error();
```

### Memory Operations

```cpp
#include <hyperv/memory.h>

namespace hyperv::memory
{
    // Physical memory operations
    bool read_physical(void* destination, std::uint64_t physical_address, std::uint64_t size);
    bool write_physical(const void* source, std::uint64_t physical_address, std::uint64_t size);
    
    // Virtual memory operations
    bool read_virtual(void* destination, std::uint64_t virtual_address, std::uint64_t cr3, std::uint64_t size);
    bool write_virtual(const void* source, std::uint64_t virtual_address, std::uint64_t cr3, std::uint64_t size);
    
    // Address translation
    std::uint64_t virtual_to_physical(std::uint64_t virtual_address, std::uint64_t cr3);
    std::uint64_t get_current_cr3();
}
```

### Hook System

```cpp
#include <hyperv/hook.h>

namespace hyperv::hook
{
    enum class hook_type : uint8_t
    {
        code,           // Code execution hook
        read,          // Memory read hook  
        write,         // Memory write hook
        read_write     // Memory read/write hook
    };

    struct hook_context
    {
        std::uint64_t target_address;    // Address to hook
        std::uint64_t shadow_page;       // Shadow page address
        hook_type type;                  // Type of hook
        std::vector<std::uint8_t> original_bytes;  // Original bytes
    };

    bool add_hook(const hook_context& context);
    bool remove_hook(std::uint64_t target_address);
}
```

## Usage Example

```cpp
#include <hyperv/core.h>
#include <hyperv/memory.h>
#include <iostream>

int main()
{
    // Initialize library
    if (!hyperv::initialize())
    {
        std::cout << "Failed to initialize hyperv library\n";
        return 1;
    }

    // Get current CR3
    std::uint64_t cr3 = hyperv::memory::get_current_cr3();
    std::cout << "Current CR3: 0x" << std::hex << cr3 << "\n";

    // Read some physical memory
    std::uint8_t buffer[0x1000];
    if (hyperv::memory::read_physical(buffer, 0x1000, sizeof(buffer)))
    {
        std::cout << "Successfully read physical memory\n";
    }

    // Cleanup
    hyperv::shutdown();
    return 0;
}
```

## Error Codes

```cpp
enum class error_code : uint8_t
{
    success = 0,
    initialization_failed,
    invalid_parameter,
    memory_operation_failed,
    hook_operation_failed,
    hypercall_failed
};
```

## Building

1. Add the `hyperv-core` project to your solution
2. Add `hyperv-core.lib` to your linker dependencies
3. Include the `hyperv-core/include` directory in your include paths
4. Include the `shared` directory from the original project for hypercall definitions

## Requirements

- Visual Studio 2019+ with C++20 support
- x64 platform
- Hyper-V hypervisor with hyperv-attachment loaded