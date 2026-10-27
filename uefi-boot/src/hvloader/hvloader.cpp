#include "hvloader.hpp"
#include "../hook/hook.hpp"
#include "../hyperv/hyperv.hpp"
#include "../sig/sig.hpp"
#include "../mem/page_tables.hpp"

using launch_hv_fn = void(*)(::cr3, void*, void*, ::cr3);

namespace
{
	hook launch_hv_hk;

	// returns 0th pml4e (overwritten by us, not meant to persist)
	pml4e_64 populate_page_tables(const cr3 hyperv_cr3) noexcept
	{
		// 0 will get wiped later, its just so the addr
		// space of the uefi driver stays mapped whilst we run the hook
		const pml4e_64 orig_pml4e = mem::pt::insert_identity_pml4e(hyperv_cr3, 0);

		// this pml4e will persist, 0 will not
		mem::pt::insert_identity_pml4e(hyperv_cr3, hva_identity_map_pml4e);

		return orig_pml4e;
	}

	void partially_restore_page_tables(const cr3 hyperv_cr3, const pml4e_64 orig_pml4e) noexcept
	{
		mem::pt::insert_pml4e(hyperv_cr3, orig_pml4e, 0);
	}

	[[nodiscard]] pe::image* find_hyperv_base(const void* const entry_point)
	{
		auto curr = reinterpret_cast<const std::uint8_t*>(
			reinterpret_cast<std::uintptr_t>(entry_point) & ~page_mask
		);

		while (true)
		{
			const auto img = reinterpret_cast<const pe::image*>(curr);
			const auto nt_hdrs = img->nt_hdrs();

			// not checking for dos header magic
			// as it is corrupted on some builds
			if (nt_hdrs->ok())
				return const_cast<pe::image*>(img);

			curr -= page_size;
		}
	}

	void apply_hyperv_hooks(const pe::image* const img) noexcept
	{
		
	}

	void launch_hv_detour(const ::cr3 hyperv_cr3, void* const hyperv_entry_point, void* const jmp_gadget,
	                      const ::cr3 kernel_cr3) noexcept
	{
		launch_hv_hk.disable();

		const ::cr3 orig_cr3 = mem::pt::load_identity_cr3();

		const pml4e_64 orig_pml4e = populate_page_tables(hyperv_cr3);

		// now contains identity map
		arch::set_cr3(hyperv_cr3);

		const auto img = find_hyperv_base(hyperv_entry_point);

		hyperv::place_hooks(img);

		partially_restore_page_tables(hyperv_cr3, orig_pml4e);

		arch::set_cr3(orig_cr3);

		launch_hv_hk.target<launch_hv_fn>()(hyperv_cr3, hyperv_entry_point, jmp_gadget, kernel_cr3);
	}
}

efi::status hvloader::place_hooks(pe::image* const img)
{
	const auto target = sig::hvloader_launch_hv.find(img);

	if (!target)
		return efi::not_found;

	launch_hv_hk = hook::create(target, reinterpret_cast<void*>(&launch_hv_detour));

	return efi::success;
}
