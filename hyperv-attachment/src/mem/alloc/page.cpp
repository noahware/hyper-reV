#include "page.hpp"
#include <heap.hpp>
#include <bitset.hpp>
#include <boot_info.hpp>
#include <mutex.hpp>

#include "../../arch/defs.hpp"

namespace
{
	std::uintptr_t arena_base = 0;

	cstd::bitset<hva_heap_page_count> free_pages{};
	cstd::mutex mutex;

	[[nodiscard]] constexpr std::size_t size_to_idx(const std::size_t s) noexcept
	{
		return s / page_size;
	}

	[[nodiscard]] std::size_t ptr_to_idx(const void* const ptr) noexcept
	{
		const std::size_t off = reinterpret_cast<std::uintptr_t>(ptr) - arena_base;

		return size_to_idx(off);
	}

	void mark_indexes_used(const std::size_t start, const std::size_t size) noexcept
	{
		for (std::size_t i = start; i < size; i++)
			free_pages.reset(i);
	}

	void mark_boot_used_pages(const boot_info& bi) noexcept
	{
		const std::size_t used_pages = size_to_idx(bi.usable_heap_base - bi.heap_range.base);
		
		mark_indexes_used(0, used_pages);
	}
}

void mem::page::init(const boot_info& bi) noexcept
{
	arena_base = bi.heap_range.base;

	free_pages.set();
	mark_boot_used_pages(bi);
}

void* mem::page::alloc(const std::size_t n) noexcept
{
	cstd::lock_guard lock(mutex);

	const std::size_t start = free_pages.find_first_run(n);

	if (start == free_pages.size())
		return nullptr;

	mark_indexes_used(start, n);

	return reinterpret_cast<void*>(arena_base + start * page_size);
}
