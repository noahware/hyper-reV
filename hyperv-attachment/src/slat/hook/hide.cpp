#include "hide.hpp"
#include "info.hpp"
#include "../cr3/cr3.hpp"
#include "../cr3/split.hpp"
#include "../../arch/current.hpp"
#include "../../mem/alloc/page.hpp"
#include "../../mem/map.hpp"

#include <boot_info.hpp>

namespace
{
	phys_addr zero_page_hpa = 0;

	phys_addr heap_gpa = 0;
	std::size_t heap_pages = 0;

	phys_addr ensure_zero_page_hpa() noexcept
	{
		if (zero_page_hpa)
			return zero_page_hpa;

		void* const page = mem::page::alloc(1);
		if (!page)
			return 0;

		zero_page_hpa = mem::phys_for_map(page);
		return zero_page_hpa;
	}

	bool hide_in(const cr3 root, const phys_addr gpa,
	             const std::size_t page_count, const phys_addr zero_pfn) noexcept
	{
		for (std::size_t i = 0; i < page_count; i++)
		{
			auto* pte = slat::cr3::find_split_leaf(root, gpa + i * page_size);
			if (!pte)
				return false;

			pte->page_frame_number = zero_pfn;
		}

		return true;
	}
}

void slat::hook::init_hide(const boot_info& bi) noexcept
{
	heap_gpa = bi.heap_range.base;
	heap_pages = bi.heap_range.size / page_size;
}

bool slat::hook::hide(const phys_addr gpa, const std::size_t page_count) noexcept
{
	cstd::lock_guard guard(hook::mtx);

	const auto zero_hpa = ensure_zero_page_hpa();
	if (!zero_hpa)
		return false;

	const auto zero_pfn = zero_hpa >> page_shift;

	if (!hide_in(cr3::hyperv, gpa, page_count, zero_pfn))
		return false;

	// and the shadow too, once it exists: before that there is nothing to hide it from,
	// and a copy taken later picks this up on its own
	if (cr3::shadow.flags && !hide_in(cr3::shadow, gpa, page_count, zero_pfn))
		return false;

	return true;
}

bool slat::hook::hide_heap() noexcept
{
	return heap_pages && hide(heap_gpa, heap_pages);
}
