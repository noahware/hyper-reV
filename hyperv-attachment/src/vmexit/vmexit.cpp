#include "vmexit.hpp"
#include "../arch/current.hpp"
#include <boot_info.hpp>

using handler_fn = std::uint64_t(*)(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t);

namespace
{
	handler_fn orig_handler = nullptr;

	bool handle_vmexit(guest_regs& regs)
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
}

std::uint64_t vmexit::detour(const std::uint64_t a1, const std::uint64_t a2,
                              const std::uint64_t a3, const std::uint64_t a4)
{
	guest_regs regs = arch::guest_regs(a1, a2);

	const auto ret = orig_handler(a1, a2, a3, a4);

	if (handle_vmexit(regs))
		arch::set_guest_regs(a1, a2, regs);

	return ret;
}
