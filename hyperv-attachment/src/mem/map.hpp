#pragma once
#include <cstdint>
#include <identity_map.hpp>

namespace mem
{
	template <class T = void*>
	[[nodiscard]] T map_phys(const std::uintptr_t addr) noexcept
	{
		return reinterpret_cast<T>(hva_identity_map_base() + addr);
	}
}
