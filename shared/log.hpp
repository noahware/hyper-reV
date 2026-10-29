#pragma once
#include <cstdint>
#include <cstddef>
#include <guest_regs.hpp>

namespace logs
{
	inline constexpr std::size_t ring_capacity = 800;

	struct entry
	{
		guest_regs regs;
		std::uint64_t rip;
		std::uint64_t cr3;
		std::uint64_t stack_data[5];
	};
}
