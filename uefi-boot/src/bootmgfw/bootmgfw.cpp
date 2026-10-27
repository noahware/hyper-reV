#include "bootmgfw.hpp"
#include "../efi/boot.hpp"
#include "../efi/file.hpp"
#include "../efi/image.hpp"
#include "../hook/hook.hpp"
#include "../hook/load_pe_img.hpp"
#include "../sig/sig.hpp"
#include "../util/log.hpp"
#include "../winload/winload.hpp"
#include <pe/image.hpp>

extern "C" {
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
}

#define BOOTMGFW_PATH L"\\efi\\microsoft\\boot\\bootmgfw.efi"
#define ORIG_BOOTMGFW_PATH L"\\efi\\microsoft\\boot\\bootmgfw.original.efi"

namespace
{
    hook load_pe_image_hk;

	[[nodiscard]] efi::status restore_file_attr(efi::file& boot_file, const efi::file& orig_file) noexcept
	{
        auto boot_info = boot_file.get_info();
        const auto orig_info = orig_file.get_info();

        if (!boot_info)
            return boot_info.error();

        if (!orig_info)
            return orig_info.error();

        (*boot_info)->create_time = (*orig_info)->create_time;
        (*boot_info)->modification_time = (*orig_info)->modification_time;

        return boot_file.set_info(*boot_info);
	}

	[[nodiscard]] efi::status restore_file(efi::file& boot_file, efi::file& orig_file) noexcept
	{
        const auto orig_buf = orig_file.read();

        if (!orig_buf)
            return orig_buf.error();

        efi::status stat = boot_file.write(orig_buf.value());

        if (!stat)
            return stat;

        stat = restore_file_attr(boot_file, orig_file);

        if (!stat)
            return stat;

        return orig_file.delete_();
	}

    std::uint64_t load_pe_image_detour(bl_file_info* file_info, std::int32_t a2, pe::image** img,
                                       std::uint32_t* img_size, std::uint64_t* a5, std::uint32_t* a6, std::uint32_t* a7,
                                       std::uint64_t a8, std::uint64_t a9, std::uint64_t a10, std::uint64_t a11,
                                       std::uint64_t a12, std::uint64_t a13, std::uint64_t a14,
                                       std::uint64_t a15) noexcept
	{
        load_pe_image_hk.disable();

        const auto ret_val = load_pe_image_hk.target<load_pe_img_fn>()(file_info, a2, img, img_size, a5, a6, a7, a8, a9, a10,
                                                                 a11, a12, a13, a14, a15);

        if (file_info->name().contains(L"winload.efi"))
        {
            if (winload::place_hooks(*img))
                LOG(L"successfully placed winload hooks");
            else
                LOG("failed to place winload hooks");
        }
        else
        {
            load_pe_image_hk.enable();
        }

        return ret_val;
	}

	[[nodiscard]] efi::status place_hooks(const efi::image& bootmgfw) noexcept
	{
        const auto img = bootmgfw.pe();
        const auto load_pe_img = sig::load_pe_img.find(img);

        if (!load_pe_img)
            return efi::not_found;

        load_pe_image_hk = hook::create(load_pe_img, reinterpret_cast<void*>(&load_pe_image_detour));

        return efi::success;
	}

	[[nodiscard]] efi::status start_hooked() noexcept
	{
        EFI_DEVICE_PATH_PROTOCOL* const dev_path = FileDevicePath(
            efi::boot_device, reinterpret_cast<const CHAR16*>(BOOTMGFW_PATH));

        if (!dev_path)
            return efi::status{ EFI_OUT_OF_RESOURCES };

        auto boot_img = efi::image::load(dev_path);

        gBS->FreePool(dev_path);

        if (!boot_img)
            return boot_img.error();

        const efi::status stat = place_hooks(*boot_img);

        if (!stat)
            return stat;

        return boot_img->start();
	}
}

efi::status bootmgfw::restore_and_start_hooked()
{
    auto orig_file = efi::boot_partition.open(ORIG_BOOTMGFW_PATH);
    auto boot_file = efi::boot_partition.open(BOOTMGFW_PATH);

    if (!orig_file || !boot_file)
        return efi::not_found;

    const efi::status stat = restore_file(*boot_file, *orig_file);

    if (!stat)
        return stat;

    return start_hooked();
}
