#include "boot.hpp"
#include "image.hpp"

extern "C" {
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>
}

namespace
{
	std::size_t query_cpu_count() noexcept
	{
		EFI_MP_SERVICES_PROTOCOL* mp = nullptr;

		if (gBS->LocateProtocol(&gEfiMpServiceProtocolGuid, nullptr, reinterpret_cast<void**>(&mp)) != EFI_SUCCESS)
			return 0;

		UINTN total = 0, enabled = 0;

		if (mp->GetNumberOfProcessors(mp, &total, &enabled) != EFI_SUCCESS)
			return 0;

		return static_cast<std::size_t>(total);
	}
}

efi::status efi::find_boot_info() noexcept
{
	const auto curr = image::curr();
	const auto curr_info = curr.info();

	if (!curr_info)
		return curr_info.error();

	const handle dev = (*curr_info)->device_handle;

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* native = nullptr;

	const status stat = gBS->HandleProtocol(dev, &gEfiSimpleFileSystemProtocolGuid,
	                                        reinterpret_cast<void**>(&native));

	if (!stat)
		return stat;

	file_system fs{ native };

	auto vol = fs.volume();

	if (!vol)
		return vol.error();

	boot_partition = cstd::move(*vol);
	boot_device = dev;
	curr_phys_base = (*curr_info)->image_base;
	curr_img_size = (*curr_info)->image_size;
	cpu_count = query_cpu_count();

	return success;
}
