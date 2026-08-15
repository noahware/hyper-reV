#pragma once
#include <cstdint>

namespace kernel_detour_holder
{
	void set_up(std::uint64_t holder_base, std::uint64_t holder_size);

	void* allocate_memory(std::uint16_t size);
	void free_memory(void* buffer);

	std::uint16_t get_allocation_offset(const void* buffer);
	void* get_allocation_from_offset(std::uint16_t offset);

	union detour_entry_t
	{
		std::uint16_t value;

		struct
		{
			std::uint16_t size : 15;
			std::uint16_t is_allocated : 1;
		};

		detour_entry_t* next();
		detour_entry_t* split(std::uint16_t size_of_next_entry);
	};

	inline detour_entry_t* list_head = nullptr;
	inline std::uint64_t holder_end = 0;
}
