#include "guest.hpp"
#include "map.hpp"
#include <crt.hpp>
#include <utility.hpp>

phys_addr mem::guest::translate_va(const cr3 cr3, const std::uintptr_t va_raw) noexcept
{
	const virt_addr va{ .val = va_raw };
	const auto pml4 = mem::map_phys<const pml4e_64*>(cr3.address_of_page_directory << page_shift);
	const auto pml4e = pml4[va.pml4e];
	if (!pml4e.present)
		return 0;

	const auto pdpt = mem::map_phys<const pdpte_64*>(pml4e.page_frame_number << page_shift);
	const auto pdpte = pdpt[va.pdpte];
	if (!pdpte.present)
		return 0;

	if (pdpte.large_page)
	{
		const pdpte_1gb_64 large{ .flags = pdpte.flags };
		return (large.page_frame_number << large_page_1gb_shift)
		       | (va.pde << large_page_shift) | (va.pte << page_shift) | va.off;
	}

	const auto pd = mem::map_phys<const pde_64*>(pdpte.page_frame_number << page_shift);
	const auto pde = pd[va.pde];
	if (!pde.present)
		return 0;

	if (pde.large_page)
	{
		const pde_2mb_64 large{ .flags = pde.flags };
		return (large.page_frame_number << large_page_shift) | (va.pte << page_shift) | va.off;
	}

	const auto ptbl = mem::map_phys<const pte_64*>(pde.page_frame_number << page_shift);
	const auto pte = ptbl[va.pte];
	if (!pte.present)
		return 0;

	return (pte.page_frame_number << page_shift) | va.off;
}

bool mem::guest::copy_phys(const std::uintptr_t src, const std::uintptr_t dest,
                           const std::uint64_t size) noexcept
{
	auto remaining = size;
	auto curr_src = src;
	auto curr_dest = dest;

	while (remaining > 0)
	{
		const auto src_page_left = page_size - (curr_src & page_mask);
		const auto dest_page_left = page_size - (curr_dest & page_mask);
		const auto chunk = cstd::min(remaining, cstd::min(src_page_left, dest_page_left));

		const auto* src_ptr = mem::map_phys<const std::uint8_t*>(curr_src);
		auto* dest_ptr = mem::map_phys<std::uint8_t*>(curr_dest);

		cstd::crt::memcpy(dest_ptr, src_ptr, chunk);

		curr_src += chunk;
		curr_dest += chunk;
		remaining -= chunk;
	}

	return true;
}

bool mem::guest::copy_virt(const cr3 src_cr3, const std::uintptr_t src,
                           const cr3 dest_cr3, const std::uintptr_t dest,
                           const std::uint64_t size) noexcept
{
	auto remaining = size;
	auto curr_src = src;
	auto curr_dest = dest;

	while (remaining > 0)
	{
		const auto src_pa = translate_va(src_cr3, curr_src);
		if (!src_pa)
			return false;

		const auto dest_pa = translate_va(dest_cr3, curr_dest);
		if (!dest_pa)
			return false;

		const auto src_page_left = page_size - (curr_src & page_mask);
		const auto dest_page_left = page_size - (curr_dest & page_mask);
		const auto chunk = cstd::min(remaining, cstd::min(src_page_left, dest_page_left));

		const auto* src_ptr = mem::map_phys<const std::uint8_t*>(src_pa);
		auto* dest_ptr = mem::map_phys<std::uint8_t*>(dest_pa);

		cstd::crt::memcpy(dest_ptr, src_ptr, chunk);

		curr_src += chunk;
		curr_dest += chunk;
		remaining -= chunk;
	}

	return true;
}
