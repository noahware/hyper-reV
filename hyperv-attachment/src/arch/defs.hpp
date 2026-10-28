#pragma once
#include <cstdint>
#include <cstddef>

using phys_addr = std::uint64_t;

#pragma warning(push)
#pragma warning(disable: 4201)
union virt_addr
{
	std::uint64_t val;

	struct
	{
		std::uint64_t off : 12;
		std::uint64_t pte : 9;
		std::uint64_t pde : 9;
		std::uint64_t pdpte : 9;
		std::uint64_t pml4e : 9;
		std::uint64_t reserved : 16;
	};
};
#pragma warning(pop)

inline constexpr std::size_t page_shift = 12;
inline constexpr std::size_t large_page_shift = 21;
inline constexpr std::size_t large_page_1gb_shift = 30;
inline constexpr std::size_t page_size = 0x1000;
inline constexpr std::size_t page_mask = page_size - 1;
