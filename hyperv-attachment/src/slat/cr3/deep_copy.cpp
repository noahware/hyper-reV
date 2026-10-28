#include "deep_copy.hpp"
#include "../../mem/map.hpp"
#include "../../mem/alloc/page.hpp"

namespace
{
	template <typename T>
	[[nodiscard]] T* alloc_slat_page() noexcept
	{
		return static_cast<T*>(mem::page::alloc());
	}

	template <typename Entry>
	void copy_level(const Entry* src, Entry* dest) noexcept
	{
		for (std::size_t i = 0; i < paging_entry_count; i++)
		{
			auto& dest_ent = dest[i];
			dest_ent.flags = src[i].flags;

			if (arch::slat_is_present(dest_ent))
			{
				const auto perm = arch::slat_perm(dest_ent) & ~page_perm::execute;
				arch::slat_set_perm(dest_ent, perm);
			}
		}
	}

	template <typename Entry, typename Child, typename... Rest>
	void copy_level(const Entry* src, Entry* dest) noexcept
	{
		for (std::size_t i = 0; i < paging_entry_count; i++)
		{
			const auto& src_ent = src[i];
			auto& dest_ent = dest[i];

			dest_ent.flags = src_ent.flags;

			if (!arch::slat_is_present(src_ent))
				continue;

			if constexpr (requires(const Entry& e) { arch::slat_is_large(e); })
			{
				if (arch::slat_is_large(src_ent))
				{
					const auto perm = arch::slat_perm(dest_ent) & ~page_perm::execute;
					arch::slat_set_perm(dest_ent, perm);
					continue;
				}
			}

			const auto* src_child = mem::map_phys<const Child*>(
				src_ent.page_frame_number << page_shift);
			auto* dest_child = alloc_slat_page<Child>();

			dest_ent.page_frame_number = mem::pfn_for_map(dest_child);

			copy_level<Child, Rest...>(src_child, dest_child);
		}
	}
}

void slat::cr3::deep_copy(const arch::slat_pml4e* const src_pml4,
                           arch::slat_pml4e* const dest_pml4) noexcept
{
	copy_level<arch::slat_pml4e, arch::slat_pdpte, arch::slat_pde, arch::slat_pte>(
		src_pml4, dest_pml4);
}
