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
	cstd::mutex mut;

	[[nodiscard]] constexpr std::size_t size_to_idx(const std::size_t s) noexcept
	{
		return s / page_size;
	}

	void mark_boot_used_pages(const boot_info& bi) noexcept
	{
		const std::size_t used_pages = size_to_idx(bi.usable_heap_base - bi.heap_range.base);
		for (std::size_t i = 0; i < used_pages; i++)
			free_pages.reset(i);
	}
}

void mem::page::init(const boot_info& bi) noexcept
{
	arena_base = bi.heap_range.base;

	mark_boot_used_pages(bi);
}
