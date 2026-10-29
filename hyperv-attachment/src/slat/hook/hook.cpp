#include "hook.hpp"
#include "info.hpp"
#include "../cr3/cr3.hpp"
#include "../cr3/split.hpp"
#include "../../arch/current.hpp"
#include "../../mem/alloc/page.hpp"
#include "../../mem/map.hpp"
#include "cache.hpp"

namespace
{
	constexpr phys_addr large_page_2mb_mask = (1ULL << large_page_shift) - 1;

	phys_addr zero_page_hpa = 0;

	bool hook_in_merge_range(const phys_addr gpa) noexcept
	{
		const auto base = gpa & ~large_page_2mb_mask;

		for (const auto& entry : slat::hook::list)
		{
			if ((entry.gpa & ~large_page_2mb_mask) == base)
				return true;
		}

		return false;
	}

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
}

bool slat::hook::add(const phys_addr gpa, const phys_addr shadow_hpa) noexcept
{
	{
		cstd::lock_guard guard(hook::mtx);

		auto* orig_pte = cr3::find_split_leaf(cr3::hyperv, gpa);
		if (!orig_pte)
			return false;

		auto* shadow_pte = cr3::find_split_leaf(cr3::shadow, gpa);
		if (!shadow_pte)
			return false;

		const auto original_hpa = orig_pte->page_frame_number << page_shift;

		arch::slat_set_perm(*orig_pte, page_perm::rw);

		shadow_pte->page_frame_number = shadow_hpa >> page_shift;
		arch::slat_set_perm(*shadow_pte, page_perm::rwx);

		list.push_front({ gpa, original_hpa, shadow_hpa });
	}

	cache::flush_all();
	return true;
}

bool slat::hook::hide(const phys_addr gpa, const std::size_t page_count) noexcept
{
	cstd::lock_guard guard(hook::mtx);

	const auto zero_hpa = ensure_zero_page_hpa();
	if (!zero_hpa)
		return false;

	const auto zero_pfn = zero_hpa >> page_shift;

	for (std::size_t i = 0; i < page_count; i++)
	{
		const phys_addr target = gpa + i * page_size;

		auto* orig_pte = cr3::find_split_leaf(cr3::hyperv, target);
		if (!orig_pte)
			return false;

		auto* shadow_pte = cr3::find_split_leaf(cr3::shadow, target);
		if (!shadow_pte)
			return false;

		orig_pte->page_frame_number = zero_pfn;
		shadow_pte->page_frame_number = zero_pfn;
	}

	return true;
}

bool slat::hook::remove(const phys_addr gpa) noexcept
{
	bool found = false;

	{
		cstd::lock_guard guard(hook::mtx);

		list.remove_if([gpa, &found](const info& entry)
		{
			if (entry.gpa != gpa)
				return false;

			found = true;

			if (auto* orig_pte = cr3::find_split_leaf(cr3::hyperv, gpa))
				arch::slat_set_perm(*orig_pte, page_perm::rwx);

			if (auto* shadow_pte = cr3::find_split_leaf(cr3::shadow, gpa))
			{
				shadow_pte->page_frame_number = entry.original_hpa >> page_shift;
				arch::slat_set_perm(*shadow_pte, page_perm::rwx);
			}

			return true;
		});

		if (found && !hook_in_merge_range(gpa))
		{
			cr3::merge(cr3::hyperv, gpa);
			cr3::merge(cr3::shadow, gpa);
		}
	}

	if (found)
		cache::flush_all();

	return found;
}
