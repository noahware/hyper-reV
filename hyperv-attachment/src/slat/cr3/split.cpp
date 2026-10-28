#include "split.hpp"
#include "../../mem/map.hpp"
#include "../../mem/alloc/page.hpp"

namespace
{
	bool split_2mb(arch::slat_pde& pde_ref) noexcept
	{
		const auto large = reinterpret_cast<const arch::slat_pde_2mb&>(pde_ref);

		auto* pt = static_cast<arch::slat_pte*>(mem::page::alloc());
		if (!pt)
			return false;

		for (std::uint64_t i = 0; i < paging_entry_count; i++)
			arch::slat_init_split_pte(pt[i], large, i);

		arch::slat_set_table_pde(pde_ref, mem::pfn_for_map(pt));
		return true;
	}

	bool split_1gb(arch::slat_pdpte& pdpte_ref) noexcept
	{
		const auto large = reinterpret_cast<const arch::slat_pdpte_1gb&>(pdpte_ref);

		auto* pd = static_cast<arch::slat_pde_2mb*>(mem::page::alloc());
		if (!pd)
			return false;

		for (std::uint64_t i = 0; i < paging_entry_count; i++)
			arch::slat_init_split_pde(pd[i], large, i);

		arch::slat_set_table_pdpte(pdpte_ref, mem::pfn_for_map(pd));
		return true;
	}
}

arch::slat_pte* slat::cr3::find_split_leaf(const ::cr3 slat_cr3, const phys_addr gpa) noexcept
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

	if (!arch::slat_is_present(pdpte))
		return nullptr;

	if (arch::slat_is_large(pdpte))
	{
		if (!split_1gb(pdpte))
			return nullptr;
	}

	auto* pd = mem::map_phys<arch::slat_pde*>(
		pdpte.page_frame_number << page_shift);
	auto& pde = pd[va.pde];

	if (!arch::slat_is_present(pde))
		return nullptr;

	if (arch::slat_is_large(pde))
	{
		if (!split_2mb(pde))
			return nullptr;
	}

	auto* pt = mem::map_phys<arch::slat_pte*>(
		pde.page_frame_number << page_shift);

	return &pt[va.pte];
}

bool slat::cr3::merge(const ::cr3 slat_cr3, const phys_addr gpa) noexcept
{
	const virt_addr va{ .val = gpa };

	auto* pml4 = mem::map_phys<arch::slat_pml4e*>(
		slat_cr3.address_of_page_directory << page_shift);
	auto& pml4e = pml4[va.pml4e];

	if (!arch::slat_is_present(pml4e))
		return false;

	auto* pdpt = mem::map_phys<arch::slat_pdpte*>(
		pml4e.page_frame_number << page_shift);
	auto& pdpte = pdpt[va.pdpte];

	if (!arch::slat_is_present(pdpte) || arch::slat_is_large(pdpte))
		return false;

	auto* pd = mem::map_phys<arch::slat_pde*>(
		pdpte.page_frame_number << page_shift);
	auto& pde = pd[va.pde];

	if (!arch::slat_is_present(pde) || arch::slat_is_large(pde))
		return true;

	const auto pt_phys = pde.page_frame_number << page_shift;

	auto* pt = mem::map_phys<arch::slat_pte*>(pt_phys);

	auto& large = reinterpret_cast<arch::slat_pde_2mb&>(pde);
	arch::slat_init_merge_pde(large, pt[va.pte]);

	mem::page::free(mem::map_phys(pt_phys));
	return true;
}
