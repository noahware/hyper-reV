#include "kernel_detour_holder.h"

void kernel_detour_holder::set_up(const std::uint64_t holder_base, const std::uint64_t holder_size)
{
	list_head = reinterpret_cast<detour_entry_t*>(holder_base);

	list_head->size = static_cast<std::uint16_t>(holder_size - sizeof(detour_entry_t));
	list_head->is_allocated = 0;

	holder_end = holder_base + holder_size;
}

void* kernel_detour_holder::allocate_memory(const std::uint16_t size)
{
	detour_entry_t* entry = list_head;
	detour_entry_t* best_split = nullptr;

	while (entry != nullptr)
	{
		if (entry->is_allocated == 0)
		{
			if (entry->size == size)
			{
				entry->is_allocated = 1;

				return ++entry;
			}

			if (size + sizeof(detour_entry_t) < entry->size)
			{
				if (best_split == nullptr || entry->size < best_split->size)
				{
					best_split = entry;
				}
			}
		}

		entry = entry->next();
	}

	if (best_split != nullptr)
	{
		detour_entry_t* split_entry = best_split->split(size);

		if (split_entry != nullptr)
		{
			return ++split_entry;
		}
	}

	return nullptr;
}

std::uint16_t kernel_detour_holder::get_allocation_offset(const void* const buffer)
{
	const auto base = reinterpret_cast<std::uint64_t>(list_head);
	const auto allocation = reinterpret_cast<std::uint64_t>(buffer);

	const auto offset = static_cast<std::uint16_t>(allocation - base);

	return offset;
}

void* kernel_detour_holder::get_allocation_from_offset(const std::uint16_t offset)
{
	const auto base = reinterpret_cast<std::uint64_t>(list_head);
	const std::uint64_t allocation = base + offset;

	return reinterpret_cast<void*>(allocation);
}

static void try_merge_of_next_entry(kernel_detour_holder::detour_entry_t* current_entry)
{
	const auto* const next = current_entry->next();

	if (next != nullptr && next->is_allocated == 0)
	{
		current_entry->size = next->size + sizeof(kernel_detour_holder::detour_entry_t);
	}
}

void kernel_detour_holder::free_memory(void* const buffer)
{
	if (buffer == nullptr)
	{
		return;
	}

	detour_entry_t* const entry = static_cast<detour_entry_t*>(buffer) - 1;

	entry->is_allocated = 0;

	try_merge_of_next_entry(entry);
}

kernel_detour_holder::detour_entry_t* kernel_detour_holder::detour_entry_t::next()
{
	const std::uint64_t next_entry = reinterpret_cast<std::uint64_t>(this + 1) + this->size;

	if (holder_end <= next_entry)
	{
		return nullptr;
	}

	return reinterpret_cast<detour_entry_t*>(next_entry);
}

// note for future: if we ever need any size above 0x1000 (at the time of writing, we do not)
// then we will start changing the shrink direction by the size
// if the allocation is less than or equal to 4kb, then it will be shrank from 1 direction,
// if its bigger than 4kb, then it will shrink from the other direction
// this will allow us to manage the memory better, so when we search for deallocated entries of the same size,
// we wont have to search as far
kernel_detour_holder::detour_entry_t* kernel_detour_holder::detour_entry_t::split(
	const std::uint16_t size_of_next_entry)
{
	const std::uint16_t needed_size = size_of_next_entry + sizeof(detour_entry_t);

	if (this->is_allocated == 1 || this->size <= needed_size)
	{
		return nullptr;
	}

	this->size -= needed_size;

	detour_entry_t* next_entry = this->next();

	next_entry->is_allocated = 1;
	next_entry->size = size_of_next_entry;

	return next_entry;
}
