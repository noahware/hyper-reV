#pragma once
#include <cstdint>

struct boot_info;

namespace vmexit
{
	void init(const boot_info& bi);

	std::uint64_t detour(std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4);
}
