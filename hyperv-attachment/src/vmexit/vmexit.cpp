#include "vmexit.hpp"
#include <boot_info.hpp>

using vmexit_handler_fn = std::uint64_t(*)(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t);

namespace
{
	vmexit_handler_fn orig_handler = nullptr;
}

void vmexit::init(const boot_info& bi)
{
	orig_handler = reinterpret_cast<vmexit_handler_fn>(bi.orig_vmexit_handler);
}

std::uint64_t vmexit::handler(const std::uint64_t a1, const std::uint64_t a2, const std::uint64_t a3,
                              const std::uint64_t a4)
{
	return orig_handler(a1, a2, a3, a4);
}
