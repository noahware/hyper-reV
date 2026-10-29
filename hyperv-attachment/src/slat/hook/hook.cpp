#include "hook.hpp"
#include "info.hpp"
#include "../cr3/cr3.hpp"
#include "../cr3/split.hpp"
#include "../../arch/current.hpp"
#include "cache.hpp"

namespace
{
	constexpr phys_addr large_page_2mb_mask = (1ULL << large_page_shift) - 1;

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
}

bool slat::hook::add(const phys_addr gpa, const phys_addr shadow_hpa) noexcept
{
	{
		cstd::lock_guard guard(hook::mtx);

		if (list.empty())
			cr3::first_hook();

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
