#pragma once
#include <cstdint>

namespace hyperv::memory
{
uint64_t read_physical(void *destination, std::uint64_t physical_address, std::uint64_t size);
uint64_t write_physical(const void *source, std::uint64_t physical_address, std::uint64_t size);

uint64_t read_host_physical(void *destination, std::uint64_t physical_address, std::uint64_t size);
uint64_t write_host_physical(const void *source, std::uint64_t physical_address, std::uint64_t size);

uint64_t read_virtual(void *destination, std::uint64_t virtual_address, std::uint64_t cr3, std::uint64_t size);
uint64_t write_virtual(const void *source, std::uint64_t virtual_address, std::uint64_t cr3, std::uint64_t size);

std::uint64_t virtual_to_physical(std::uint64_t virtual_address, std::uint64_t cr3);
std::uint64_t get_current_cr3();
} // namespace hyperv::memory