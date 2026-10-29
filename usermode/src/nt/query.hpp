#pragma once
#include <cstdint>
#include <vector>

namespace nt
{
	constexpr std::uint32_t system_module_information_class = 11;

	// Calls NtQuerySystemInformation with a size probe, allocates, then fills.
	// Returns an empty vector on failure.
	std::vector<std::uint8_t> query_system_info(std::uint32_t info_class) noexcept;
}
