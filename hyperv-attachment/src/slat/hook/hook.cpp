#include "hook.hpp"
#include "info.hpp"
#include "../cr3/cr3.hpp"
#include "../../mem/map.hpp"
#include "../../arch/current.hpp"
#include "cache.hpp"

namespace
{
	arch::slat_pte* find_leaf(const ::cr3 slat_cr3, const phys_addr gpa) noexcept
	{
		const virt_addr va{ .val = gpa };

		auto* pml4 = mem::map_phys<arch::slat_pml4e*>(
			slat_cr3.address_of_page_directory << page_shift);
		auto& pml4e = pml4[va.pml4e];

		if (!arch::slat_is_present(pml4e))
			return nullptr;

		auto* pdpt = mem::map_phys<arch::slat_pdpte*>(
			pml4e.page_frame_number << page_shift);
		auto& pdpte = pdpt[va.pdpte];

		if (!arch::slat_is_present(pdpte) || arch::slat_is_large(pdpte))
			return nullptr;

		auto* pd = mem::map_phys<arch::slat_pde*>(
			pdpte.page_frame_number << page_shift);
		auto& pde = pd[va.pde];

		if (!arch::slat_is_present(pde) || arch::slat_is_large(pde))
			return nullptr;

		auto* pt = mem::map_phys<arch::slat_pte*>(
			pde.page_frame_number << page_shift);

		return &pt[va.pte];
	}
}

void slat::hook::add(const phys_addr gpa, const phys_addr shadow_hpa) noexcept
{
	{
		cstd::lock_guard guard(hook::mtx);

		if (list.empty())
			cr3::first_hook();

		auto* orig_pte = find_leaf(cr3::hyperv_cr3, gpa);
		if (!orig_pte)
			return;

		auto* shadow_pte = find_leaf(cr3::shadow_cr3, gpa);
		if (!shadow_pte)
			return;

		const auto original_hpa = orig_pte->page_frame_number << page_shift;

		arch::slat_set_perm(*orig_pte, page_perm::rw);

		shadow_pte->page_frame_number = shadow_hpa >> page_shift;
		arch::slat_set_perm(*shadow_pte, page_perm::rwx);

		list.push_front({ gpa, original_hpa, shadow_hpa });
	}

	slat::cache::flush_all();
}

void slat::hook::remove(const phys_addr gpa) noexcept
{
	{
		cstd::lock_guard guard(hook::mtx);

		list.remove_if([gpa](const info& entry)
		{
			if (entry.gpa != gpa)
				return false;

			auto* orig_pte = find_leaf(cr3::hyperv_cr3, gpa);
			if (orig_pte)
				arch::slat_set_perm(*orig_pte, page_perm::rwx);

			auto* shadow_pte = find_leaf(cr3::shadow_cr3, gpa);
			if (shadow_pte)
			{
				shadow_pte->page_frame_number = entry.original_hpa >> page_shift;
				arch::slat_set_perm(*shadow_pte, page_perm::rwx);
			}

			return true;
		});
	}

	slat::cache::flush_all();
}
