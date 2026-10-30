#pragma once
#include "../arch/defs.hpp"
#include "map.hpp"
#include <ia32.hpp>
#include <crt.hpp>
#include <utility.hpp>

namespace mem
{
	template <typename MapFn>
	phys_addr translate_va(const cr3 cr3, const std::uintptr_t va_raw, MapFn map_pa) noexcept
	{
		const virt_addr va{ .val = va_raw };

		const auto* pml4 = static_cast<const pml4e_64*>(
			map_pa(cr3.address_of_page_directory << page_shift));
		if (!pml4)
			return 0;
		const auto& pml4e = pml4[va.pml4e];
		if (!pml4e.present)
			return 0;

		const auto* pdpt = static_cast<const pdpte_64*>(
			map_pa(pml4e.page_frame_number << page_shift));
		if (!pdpt)
			return 0;
		const auto& pdpte = pdpt[va.pdpte];
		if (!pdpte.present)
			return 0;

		if (pdpte.large_page)
		{
			const pdpte_1gb_64 large{ .flags = pdpte.flags };
			return (large.page_frame_number << large_page_1gb_shift)
			       | (va.pde << large_page_shift) | (va.pte << page_shift) | va.off;
		}

		const auto* pd = static_cast<const pde_64*>(
			map_pa(pdpte.page_frame_number << page_shift));
		if (!pd)
			return 0;
		const auto& pde = pd[va.pde];
		if (!pde.present)
			return 0;

		if (pde.large_page)
		{
			const pde_2mb_64 large{ .flags = pde.flags };
			return (large.page_frame_number << large_page_shift) | (va.pte << page_shift) | va.off;
		}

		const auto* ptbl = static_cast<const pte_64*>(
			map_pa(pde.page_frame_number << page_shift));
		if (!ptbl)
			return 0;
		const auto& pte = ptbl[va.pte];
		if (!pte.present)
			return 0;

		return (pte.page_frame_number << page_shift) | va.off;
	}

	inline phys_addr translate_va(const cr3 cr3, const std::uintptr_t va) noexcept
	{
		return translate_va(cr3, va, [](std::uintptr_t pa) -> void* {
			return map_phys(pa);
		});
	}

	bool copy_phys(std::uintptr_t src, std::uintptr_t dest, std::uint64_t size) noexcept;

	template <typename ResolveFn>
	bool copy_virt(const cr3 src_cr3, const std::uintptr_t src,
	               const cr3 dest_cr3, const std::uintptr_t dest,
	               const std::uint64_t size, ResolveFn resolve) noexcept
	{
		auto remaining = size;
		auto curr_src = src;
		auto curr_dest = dest;

		while (remaining > 0)
		{
			const auto* src_ptr = static_cast<const std::uint8_t*>(resolve(src_cr3, curr_src));
			if (!src_ptr)
				return false;

			auto* dest_ptr = static_cast<std::uint8_t*>(resolve(dest_cr3, curr_dest));
			if (!dest_ptr)
				return false;

			const auto src_page_left = page_size - (curr_src & page_mask);
			const auto dest_page_left = page_size - (curr_dest & page_mask);
			const auto chunk = cstd::min(remaining, cstd::min(src_page_left, dest_page_left));

			cstd::crt::memcpy(dest_ptr, src_ptr, chunk);

			curr_src += chunk;
			curr_dest += chunk;
			remaining -= chunk;
		}

		return true;
	}

	inline bool copy_virt(const cr3 src_cr3, const std::uintptr_t src,
	                       const cr3 dest_cr3, const std::uintptr_t dest,
	                       const std::uint64_t size) noexcept
	{
		return copy_virt(src_cr3, src, dest_cr3, dest, size,
			[](const cr3 c, const std::uintptr_t va) -> void* {
				const auto pa = translate_va(c, va);
				return pa ? map_phys(pa) : nullptr;
			});
	}
}
