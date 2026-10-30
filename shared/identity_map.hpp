#pragma once
#include <cstdint>
#include <cstddef>

inline constexpr std::size_t hva_identity_map_pml4e = 255;

constexpr std::uintptr_t hva_identity_map_base()
{
	constexpr std::size_t pml4e_shift = 39;

	return hva_identity_map_pml4e << pml4e_shift;
}
