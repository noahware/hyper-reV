#include "vmexit.hpp"
#include "../arch/current.hpp"
#include "../mem/map.hpp"
#include <boot_info.hpp>
#include <atomic.hpp>
#include <crt.hpp>

using handler_fn = std::uint64_t(*)(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t);

namespace
{
	handler_fn orig_handler = nullptr;
	phys_range boot_img;

	void wipe_boot_img() noexcept
	{
		const auto img_base = mem::map_phys<std::uint8_t*>(boot_img.base);

		cstd::crt::memset(img_base, 0, boot_img.size);
	}

	void handle_first_vmexit() noexcept
	{
		static cstd::atomic<bool> handled;

		if (handled.exchange(true))
			return;

		wipe_boot_img();
	}

	bool handle_vmexit(guest_regs& regs) noexcept
	{
		switch (arch::exit_reason())
		{
		case exit_reason::cpuid:
			return false;
		default:
			return false;
		}
	}
}

void vmexit::init(const boot_info& bi)
{
	orig_handler = reinterpret_cast<handler_fn>(bi.orig_vmexit_handler);
	boot_img = bi.boot_img;
}

std::uint64_t vmexit::detour(const std::uint64_t a1, const std::uint64_t a2,
                              const std::uint64_t a3, const std::uint64_t a4)
{
	handle_first_vmexit();

	guest_regs regs = arch::guest_regs(a1, a2);

	const auto ret = orig_handler(a1, a2, a3, a4);

	if (handle_vmexit(regs))
		arch::set_guest_regs(a1, a2, regs);

	return ret;
}
