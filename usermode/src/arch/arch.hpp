#pragma once
#include <cstddef>
#include <cstdint>

namespace arch
{
	constexpr std::uint64_t page_size = 0x1000;

	constexpr std::size_t max_insn_size = 15;

	constexpr std::uintptr_t page_base(const std::uintptr_t va) noexcept
	{
		return va & ~(page_size - 1);
	}
}
