#include "winload.hpp"
#include "../hook/hook.hpp"
#include "../hook/load_pe_img.hpp"
#include "../hvloader/hvloader.hpp"
#include "../sig/sig.hpp"

namespace
{
	hook load_pe_image_hk;

	std::uint64_t load_pe_image_detour(bl_file_info* file_info, std::int32_t a2, pe::image** img,
	                                   std::uint32_t* img_size, std::uint64_t* a5, std::uint32_t* a6, std::uint32_t* a7,
	                                   std::uint64_t a8, std::uint64_t a9, std::uint64_t a10, std::uint64_t a11,
	                                   std::uint64_t a12, std::uint64_t a13, std::uint64_t a14,
	                                   std::uint64_t a15) noexcept
	{
		load_pe_image_hk.disable();

		const auto ret_val = load_pe_image_hk.target<load_pe_img_fn>()(file_info, a2, img, img_size, a5, a6, a7, a8, a9, a10,
		                                                                a11, a12, a13, a14, a15);

		if (file_info->name().contains(L"hvloader"))
		{
			hvloader::place_hooks(*img);
		}
		else
		{
			load_pe_image_hk.enable();
		}

		return ret_val;
	}
}

efi::status winload::place_hooks(pe::image* const img)
{
	const auto target = sig::load_pe_img.find(img);

	if (!target)
		return efi::not_found;

	load_pe_image_hk = hook::create(target, reinterpret_cast<void*>(&load_pe_image_detour));

	return efi::success;
}
