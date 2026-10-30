#include "../mem/map.hpp"

void* apic_lib_map_physical_address(const std::uint64_t physical_address)
{
	return mem::map_phys<void*>(physical_address);
}

void apic_lib_unmap_physical_address(void*)
{
}
