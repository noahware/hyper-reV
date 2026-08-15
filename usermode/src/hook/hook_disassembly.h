#pragma once
#include <vector>

union parted_address_t
{
	struct
	{
		std::uint32_t low_part;
		std::uint32_t high_part;
	} u;

	std::uint64_t value;
};

namespace hook_disasm
{
	std::pair<std::vector<std::uint8_t>, std::uint64_t> get_routine_aligned_bytes(
		std::uint8_t* routine, std::uint64_t minimum_size, std::uint64_t routine_runtime_address);
}
