#include "hyperv.hpp"
#include "../hook/hook.hpp"
#include "../hyperv_attachment/hva.hpp"
#include "../efi/boot.hpp"
#include "../mem/heap.hpp"
#include "../sig/sig.hpp"
#include <boot_info.hpp>

namespace
{
	bool is_amd = false;
	hook code_cave_hk;

	[[nodiscard]] std::uint8_t* find_vmexit_handler_and_arch(pe::image* const img) noexcept
	{
		const auto amd_vmexit_handler = sig::amd_vmexit_handler.find(img);

		is_amd = amd_vmexit_handler != nullptr;

		return is_amd ?
			amd_vmexit_handler :
			sig::intel_vmexit_handler.find(img);
	}

	[[nodiscard]] std::uint8_t* find_amd_vmcb_gadget(pe::image* const img) noexcept
	{
		if (is_amd)
			return nullptr;

		return sig::amd_vmcb_gadget.find(img);
	}

	[[nodiscard]] std::uint8_t* setup_near_trampoline(pe::image* const img, std::uint8_t* const vmexit_detour) noexcept
	{
		const auto code_cave = sig::code_cave_16.find(img);

		if (!code_cave)
			return nullptr;

		code_cave_hk = hook::create(code_cave, vmexit_detour);

		return code_cave;
	}

	void hook_vmexit_handler(std::uint8_t* const vmexit_handler, const std::uint8_t* const trampoline) noexcept
	{
		const sig::rel rel = is_amd ? *sig::amd_vmexit_handler.rel : *sig::intel_vmexit_handler.rel;

		util::patch_rel(vmexit_handler, trampoline, rel.rva_off, rel.rip_off);
	}

	[[nodiscard]] std::uint32_t find_curr_cpu_id_gs_off(pe::image* const img) noexcept
	{
		const auto base = sig::curr_cpu_id_gs_off.find(img);

		if (!base)
			return 0;

		// the 4 bytes at rva_off are the gs-relative offset the handler reads the current cpu id from
		std::uint32_t gs_off = 0;

		cstd::crt::memcpy(&gs_off, base + sig::curr_cpu_id_gs_off.rel->rva_off, sizeof(gs_off));

		return gs_off;
	}

	[[nodiscard]] boot_info form_boot_info(pe::image* const img, std::uint8_t* const vmexit_handler) noexcept
	{
		const boot_info bi = {
			.orig_vmexit_handler = vmexit_handler,
			.boot_img = {
				.base = reinterpret_cast<std::uintptr_t>(efi::curr_phys_base),
				.size = efi::curr_img_size,
			},
			.heap_range = {
				.base = mem::heap::base(),
				.size = mem::heap::size(),
			},
			.usable_heap_base = mem::heap::curr(),
			.curr_cpu_id_gs_off = find_curr_cpu_id_gs_off(img),
		};

		return bi;
	}
}

efi::status hyperv::place_hooks(pe::image* const img)
{
	std::uint8_t* const vmexit_handler = find_vmexit_handler_and_arch(img);

	if (!vmexit_handler)
		return efi::not_found;

	//std::uint8_t* const amd_vmcb_gadget = find_amd_vmcb_gadget(img);

	const boot_info bi = form_boot_info(img, vmexit_handler);

	std::uint8_t* const vmexit_detour = hva::call_entry_point(bi);

	std::uint8_t* const trampoline = setup_near_trampoline(img, vmexit_detour);

	if (!vmexit_detour || !trampoline)
		return efi::not_found;

	hook_vmexit_handler(vmexit_handler, trampoline);

	return efi::success;
}
