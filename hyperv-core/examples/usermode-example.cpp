#include <iostream>
#include <string>
#include <print>
#include <Windows.h>

// Include the new hyperv-core library
#include <hyperv/core.h>
#include <hyperv/memory.h>
#include <hyperv/hook.h>

// Keep only the necessary parts from the original usermode
// This would replace the current main.cpp and integrate with existing CLI

int main()
{
    std::cout << "Hyperv-Core Library Integration Example\n";
    std::cout << "=======================================\n";

    // Initialize the core library
    if (!hyperv::initialize())
    {
        std::println("Failed to initialize hyperv-core library");
        std::println("Make sure hyperv-attachment is loaded");
        std::system("pause");
        return 1;
    }

    std::println("Hyperv-core library initialized successfully");

    // Simple command loop using the library
    while (true)
    {
        std::print("> ");

        std::string command;
        std::getline(std::cin, command);

        if (command == "exit")
        {
            break;
        }
        else if (command == "cr3")
        {
            std::uint64_t cr3 = hyperv::memory::get_current_cr3();
            std::println("Current CR3: 0x{:x}", cr3);
        }
        else if (command.starts_with("readphys "))
        {
            // Parse address from command
            std::string addr_str = command.substr(9);
            std::uint64_t addr = std::stoull(addr_str, nullptr, 16);
            
            std::uint8_t buffer[0x10];
            if (hyperv::memory::read_physical(buffer, addr, sizeof(buffer)))
            {
                std::print("Physical memory at 0x{:x}: ", addr);
                for (int i = 0; i < sizeof(buffer); ++i)
                {
                    std::print("{:02x} ", buffer[i]);
                }
                std::println("");
            }
            else
            {
                std::println("Failed to read physical memory at 0x{:x}", addr);
            }
        }
        else if (command.starts_with("v2p "))
        {
            // Parse virtual address
            std::string addr_str = command.substr(4);
            std::uint64_t vaddr = std::stoull(addr_str, nullptr, 16);
            std::uint64_t cr3 = hyperv::memory::get_current_cr3();
            
            std::uint64_t paddr = hyperv::memory::virtual_to_physical(vaddr, cr3);
            
            if (paddr != 0)
            {
                std::println("Virtual 0x{:x} -> Physical 0x{:x}", vaddr, paddr);
            }
            else
            {
                std::println("Failed to translate virtual address 0x{:x}", vaddr);
            }
        }
        else if (command == "help")
        {
            std::println("Available commands:");
            std::println("  cr3              - Get current CR3 value");
            std::println("  readphys <addr>  - Read 16 bytes from physical address (hex)");
            std::println("  v2p <addr>       - Translate virtual to physical address (hex)");
            std::println("  help             - Show this help");
            std::println("  exit             - Exit the program");
        }
        else if (!command.empty())
        {
            std::println("Unknown command: '{}'. Type 'help' for available commands.", command);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    // Cleanup
    hyperv::shutdown();
    std::println("Hyperv-core library shutdown complete");

    return 0;
}