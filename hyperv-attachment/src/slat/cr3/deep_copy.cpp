#include "deep_copy.hpp"
#include "../../mem/map.hpp"
#include "../../mem/alloc/page.hpp"
#include <crt.hpp>

namespace
{
	template <typename T>
	[[nodiscard]] T* alloc_slat_page() noexcept
	{
		auto* page = static_cast<T*>(mem::page::alloc());
		if (page)
			cstd::crt::memset(page, 0, page_size);
		return page;
	}

	void copy_pt(const arch::slat_pte* src, arch::slat_pte* dest) noexcept
	{
		for (std::size_t i = 0; i < paging_entry_count; i++)
		{
			dest[i].flags = src[i].flags;

			if (arch::slat_is_present(dest[i]))
			{
				const auto perm = arch::slat_perm(dest[i]) & ~page_perm::execute;
				arch::slat_set_perm(dest[i], perm);
			}
		}
	}

	void copy_pd(const arch::slat_pde* src, arch::slat_pde* dest) noexcept
	{
		for (std::size_t i = 0; i < paging_entry_count; i++)
		{
			dest[i].flags = src[i].flags;

			if (!arch::slat_is_present(src[i]))
				continue;

			if (!arch::slat_is_large(src[i]))
			{
				const auto* src_pt = mem::map_phys<const arch::slat_pte*>(
					src[i].page_frame_number << page_shift);
				auto* dest_pt = alloc_slat_page<arch::slat_pte>();

				dest[i].page_frame_number = mem::unmap_phys(dest_pt) >> page_shift;

				copy_pt(src_pt, dest_pt);
			}
			else
			{
				const auto perm = arch::slat_perm(dest[i]) & ~page_perm::execute;
				arch::slat_set_perm(dest[i], perm);
			}
		}
	}

	void copy_pdpt(const arch::slat_pdpte* src, arch::slat_pdpte* dest) noexcept
	{
		for (std::size_t i = 0; i < paging_entry_count; i++)
		{
			dest[i].flags = src[i].flags;

			if (!arch::slat_is_present(src[i]))
				continue;

			if (!arch::slat_is_large(src[i]))
			{
				const auto* src_pd = mem::map_phys<const arch::slat_pde*>(
					src[i].page_frame_number << page_shift);
				auto* dest_pd = alloc_slat_page<arch::slat_pde>();

				dest[i].page_frame_number = mem::unmap_phys(dest_pd) >> page_shift;

				copy_pd(src_pd, dest_pd);
			}
			else
			{
				const auto perm = arch::slat_perm(dest[i]) & ~page_perm::execute;
				arch::slat_set_perm(dest[i], perm);
			}
		}
	}
}

void slat::cr3::deep_copy(const arch::slat_pml4e* const src_pml4,
                           arch::slat_pml4e* const dest_pml4) noexcept
{
	for (std::size_t i = 0; i < paging_entry_count; i++)
	{
		dest_pml4[i].flags = src_pml4[i].flags;

		if (!arch::slat_is_present(src_pml4[i]))
			continue;

		const auto* src_pdpt = mem::map_phys<const arch::slat_pdpte*>(
			src_pml4[i].page_frame_number << page_shift);
		auto* dest_pdpt = alloc_slat_page<arch::slat_pdpte>();

		dest_pml4[i].page_frame_number = mem::unmap_phys(dest_pdpt) >> page_shift;

		copy_pdpt(src_pdpt, dest_pdpt);
	}
}
