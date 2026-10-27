#pragma once
#include <cstdint>
#include <cstddef>

struct phys_range
{
	std::uintptr_t base;
	std::size_t size;
};

struct boot_info
{
	std::uint8_t* orig_vmexit_handler;
	std::uint8_t* vmcb_gadget;
	phys_range boot_img;
	phys_range heap_range;
	std::uintptr_t usable_heap_base;
	std::uint32_t curr_cpu_id_gs_off;
	std::size_t cpu_count;
};
