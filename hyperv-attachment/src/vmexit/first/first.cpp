#include "first.hpp"
#include "../../mem/map.hpp"
#include "../../vcpu/vcpu.hpp"
#include "../../slat/slat.hpp"
#include "../../intr/nmi.hpp"

#include <atomic.hpp>
#include <boot_info.hpp>
#include <crt.hpp>

namespace
{
	phys_range boot_img;

	void wipe_boot_img() noexcept
	{
		const auto img_base = mem::map_phys<std::uint8_t*>(boot_img.base);

		cstd::crt::memset(img_base, 0, boot_img.size);
	}
}

void vmexit::first::init(const boot_info& bi)
{
	boot_img = bi.boot_img;
}

void vmexit::first::handle()
{
	static cstd::atomic<bool> handled;

	if (handled.load(cstd::memory_order::relaxed))
		return;

	if (handled.exchange(true))
		return;

	wipe_boot_img();
	slat::init();
	intr::nmi::init();
}

void vmexit::first::handle_non_bsp()
{
	static cstd::atomic<bool> handled;

	if (handled.load(cstd::memory_order::relaxed))
		return;

	if (vcpu::is_bsp())
		return;

	if (handled.exchange(true))
		return;
}
