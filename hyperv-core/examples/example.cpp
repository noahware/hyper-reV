#include <hyperv/core.h>
#include <hyperv/memory.h>
#include <hyperv/hook.h>
#include <iostream>
#include <vector>

// Example: Basic library usage
void example_memory_operations()
{
    std::cout << "=== Memory Operations Example ===\n";

    // Get current CR3
    std::uint64_t cr3 = hyperv::memory::get_current_cr3();
    std::cout << "Current CR3: 0x" << std::hex << cr3 << std::endl;

    if (cr3 == 0)
    {
        std::cout << "Failed to get CR3\n";
        return;
    }

    // Read physical memory
    std::vector<std::uint8_t> buffer(0x1000);
    if (hyperv::memory::read_physical(buffer.data(), 0x1000, buffer.size()))
    {
        std::cout << "Successfully read physical memory at 0x1000\n";
        std::cout << "First 16 bytes: ";
        for (int i = 0; i < 16; ++i)
        {
            std::cout << std::hex << (int)buffer[i] << " ";
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Failed to read physical memory\n";
    }

    // Translate virtual to physical
    std::uint64_t test_va = 0x7FF000000000; // Example user-mode address
    std::uint64_t physical = hyperv::memory::virtual_to_physical(test_va, cr3);
    
    if (physical != 0)
    {
        std::cout << "Virtual 0x" << std::hex << test_va 
                  << " -> Physical 0x" << physical << std::endl;
    }
}

// Example: Hook system usage
void example_hook_system()
{
    std::cout << "\n=== Hook System Example ===\n";

    // Create a hook context
    hyperv::hook::hook_context hook_ctx = {};
    hook_ctx.target_address = 0xFFFFF80000000000; // Example kernel address
    hook_ctx.shadow_page = 0x12345000;            // Example shadow page
    hook_ctx.type = hyperv::hook::hook_type::code;
    
    // Example original bytes (would normally be read from target)
    hook_ctx.original_bytes = { 0x48, 0x89, 0x5C, 0x24, 0x08 }; // mov [rsp+8], rbx

    std::cout << "Attempting to add hook at 0x" << std::hex << hook_ctx.target_address << std::endl;
    
    if (hyperv::hook::add_hook(hook_ctx))
    {
        std::cout << "Hook added successfully\n";
        
        // Remove the hook
        if (hyperv::hook::remove_hook(hook_ctx.target_address))
        {
            std::cout << "Hook removed successfully\n";
        }
        else
        {
            std::cout << "Failed to remove hook\n";
        }
    }
    else
    {
        std::cout << "Failed to add hook\n";
    }
}

// Example: Error handling
void example_error_handling()
{
    std::cout << "\n=== Error Handling Example ===\n";

    // Try an invalid operation
    if (!hyperv::memory::read_physical(nullptr, 0x1000, 0x1000))
    {
        hyperv::error_code error = hyperv::get_last_error();
        std::cout << "Operation failed with error code: " << (int)error << std::endl;
        
        switch (error)
        {
        case hyperv::error_code::invalid_parameter:
            std::cout << "Error: Invalid parameter\n";
            break;
        case hyperv::error_code::memory_operation_failed:
            std::cout << "Error: Memory operation failed\n";
            break;
        default:
            std::cout << "Error: Unknown error\n";
            break;
        }
    }
}

int main()
{
    std::cout << "Hyperv-Core Library Example\n";
    std::cout << "============================\n";

    // Initialize the library
    if (!hyperv::initialize())
    {
        std::cout << "Failed to initialize hyperv library\n";
        std::cout << "Make sure hyperv-attachment is loaded\n";
        return 1;
    }

    std::cout << "Library initialized successfully\n";

    try
    {
        // Run examples
        example_memory_operations();
        example_hook_system();
        example_error_handling();
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }

    // Cleanup
    hyperv::shutdown();
    std::cout << "\nLibrary shutdown complete\n";

    return 0;
}