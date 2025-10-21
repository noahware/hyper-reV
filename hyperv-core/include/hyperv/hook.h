#pragma once
#include <cstdint>
#include <vector>

namespace hyperv::hook {
// SLAT hook types
enum class hook_type : uint8_t {
  code,       // Code execution hook
  read,       // Memory read hook
  write,      // Memory write hook
  read_write  // Memory read/write hook
};

// Hook context
struct hook_context {
  std::uint64_t target_address;              // Address being hooked
  std::uint64_t shadow_page;                 // Shadow page address
  hook_type type;                            // Type of hook
  std::vector<std::uint8_t> original_bytes;  // Original bytes at hook location
};

// Core hooking operations
bool add_hook(const hook_context& context);
bool remove_hook(std::uint64_t target_address);

// Hook management
bool initialize_hook_system();
void cleanup_hook_system();
}  // namespace hyperv::hook