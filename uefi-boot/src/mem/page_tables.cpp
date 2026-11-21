#include "page_tables.hpp"
#include "heap.hpp"
#include <crt.hpp>

namespace
{
	::cr3 identity_cr3 = { };

	constexpr std::size_t pde_size = 2ULL * 1024 * 1024;
	constexpr std::size_t identity_pd_count = 256;
}

void mem::pt::init()
{
	const phys_addr pml4_base = heap::alloc_pages(1);
	const phys_addr pdpt_base = heap::alloc_pages(1);
	const phys_addr pd_base = heap::alloc_pages(identity_pd_count);

	auto* pml4 = reinterpret_cast<pml4e_64*>(pml4_base);
	auto* pdpt = reinterpret_cast<pdpte_64*>(pdpt_base);
	auto* pds = reinterpret_cast<pde_64*>(pd_base);

	cstd::crt::memset(pml4, 0, page_size);
	cstd::crt::memset(pdpt, 0, page_size);
	cstd::crt::memset(pds, 0, identity_pd_count * page_size);

	pml4[0].page_frame_number = pdpt_base >> page_shift;
	pml4[0].present = true;
	pml4[0].write = true;

	for (std::size_t i = 0; i < pte_count; i++)
	{
		pdpt[i].page_frame_number = (pd_base + i * page_size) >> page_shift;
		pdpt[i].present = true;
		pdpt[i].write = true;

		auto* pd = reinterpret_cast<pde_64*>(pd_base + i * page_size);
		for (std::size_t j = 0; j < pte_count; j++)
		{
			const std::size_t index = i * pte_count + j;
			pd[j].page_frame_number = (index * pde_size) >> page_shift;
			pd[j].present = true;
			pd[j].write = true;
			pd[j].large_page = true;
		}
	}

	::identity_cr3.flags = 0;
	::identity_cr3.address_of_page_directory = pml4_base >> page_shift;
}

pml4e_64 mem::pt::identity_pml4e()
{
	const auto pml4 = map_phys<pml4e_64*>(::identity_cr3.address_of_page_directory << page_shift);

	return pml4[0];
}

pml4e_64 mem::pt::insert_pml4e(const cr3 cr3, const pml4e_64 pml4e, const std::size_t idx)
{
	const auto pml4 = map_phys<pml4e_64*>(cr3.address_of_page_directory << page_shift);

	const pml4e_64 orig_pml4e = pml4[idx];

	pml4[idx] = pml4e;

	return orig_pml4e;
}

pml4e_64 mem::pt::insert_identity_pml4e(const cr3 cr3, const std::size_t idx)
{
	return insert_pml4e(cr3, identity_pml4e(), idx);
}

::cr3 mem::pt::identity_cr3()
{
	return ::identity_cr3;
}

::cr3 mem::pt::load_identity_cr3()
{
	::cr3 old_cr3 = arch::cr3();

	arch::set_cr3(::identity_cr3);

	return old_cr3;
}

bool mem::pt::is_va_exec(const ::cr3 cr3, const std::uintptr_t va) noexcept
{
	const virt_addr va_bits = { .addr = reinterpret_cast<void*>(va) };

	const auto pml4 = map_phys<const pml4e_64*>(cr3.address_of_page_directory << page_shift);
	const pml4e_64 pml4e = pml4[va_bits.pml4e];
	if (!pml4e.present || pml4e.execute_disable)
		return false;

	const auto pdpt = map_phys<const pdpte_64*>(pml4e.page_frame_number << page_shift);
	const pdpte_64 pdpte = pdpt[va_bits.pdpte];
	if (!pdpte.present || pdpte.execute_disable)
		return false;
	if (pdpte.large_page) // 1 GiB leaf
		return true;

	const auto pd = map_phys<const pde_64*>(pdpte.page_frame_number << page_shift);
	const pde_64 pde = pd[va_bits.pde];
	if (!pde.present || pde.execute_disable)
		return false;
	if (pde.large_page) // 2 MiB leaf
		return true;

	const auto ptbl = map_phys<const pte_64*>(pde.page_frame_number << page_shift);
	const pte_64 pte = ptbl[va_bits.pte];
	return pte.present && !pte.execute_disable;
}
