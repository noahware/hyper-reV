#pragma once
#include <cstddef>

#include "file.hpp"
#include "obj.hpp"
#include "status.hpp"

namespace efi
{
	// the volume we booted from, resolved once at startup
	inline file boot_partition;

	// the device handle that volume lives on — needed to build device paths
	// (FileDevicePath) when loading sibling images with LoadImage
	inline handle boot_device = nullptr;

	// where our own loaded image lives in RAM (UEFI boot-services identity map,
	// so image_base IS the physical base). hva reads these so it can locate
	// this loader once it runs under hyperv's address space
	inline void* curr_phys_base = nullptr;
	inline std::uint64_t curr_img_size = 0;

	// number of logical processors firmware enumerated (EFI_MP_SERVICES_PROTOCOL)
	inline std::size_t cpu_count = 0;

	// resolves our boot volume + device handle via LoadedImage->DeviceHandle,
	// and snapshots our own image's base/size
	status find_boot_info() noexcept;
}
