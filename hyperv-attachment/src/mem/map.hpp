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

	[[nodiscard]] inline std::uintptr_t phys_for_map(const void* ptr) noexcept
	{
		return reinterpret_cast<std::uintptr_t>(ptr) - hva_identity_map_base();
	}

	[[nodiscard]] inline std::uint64_t pfn_for_map(const void* ptr) noexcept
	{
		return phys_for_map(ptr) >> page_shift;
	}
}
