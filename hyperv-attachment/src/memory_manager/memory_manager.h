#pragma once
#include <ia32-doc/ia32.hpp>
#include <structures/memory_operation.h>
#include "../structures/virtual_address.h"

namespace memory_manager
{
std::uint64_t map_host_physical(std::uint64_t host_physical_address);
std::uint64_t unmap_host_physical(std::uint64_t host_mapped_address);
std::uint64_t map_guest_physical(cr3 slat_cr3, std::uint64_t guest_physical_address,
                                 std::uint64_t *size_left_of_page = nullptr);

std::uint64_t translate_guest_virtual_address(cr3 guest_cr3, cr3 slat_cr3, virtual_address_t guest_virtual_address,
                                              std::uint64_t *size_left_of_page = nullptr);
std::uint64_t translate_host_virtual_address(cr3 host_cr3, virtual_address_t host_virtual_address,
                                             std::uint64_t *size_left_of_page = nullptr);

std::uint64_t operate_on_guest_virtual_memory(cr3 slat_cr3, void *host_buffer, std::uint64_t guest_virtual_address,
                                              cr3 guest_cr3, std::uint64_t total_size, memory_operation_t operation);
} // namespace memory_manager
