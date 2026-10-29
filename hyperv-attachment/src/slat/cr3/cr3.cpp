#include "cr3.hpp"
#include "deep_copy.hpp"
#include "../../mem/map.hpp"
#include "../../mem/alloc/page.hpp"

void slat::cr3::init() noexcept
{
	hyperv = arch::slat_cr3();
}

void slat::cr3::first_hook() noexcept
{
	const auto* src_pml4 = mem::map_phys<const arch::slat_pml4e*>(
		hyperv.address_of_page_directory << page_shift);

	auto* dest_pml4 = static_cast<arch::slat_pml4e*>(mem::page::alloc());

	deep_copy(src_pml4, dest_pml4);

	shadow = hyperv;
	shadow.address_of_page_directory = mem::pfn_for_map(dest_pml4);
}
