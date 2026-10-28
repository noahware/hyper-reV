#pragma once
#include <cstdint>

enum class exit_reason : std::uint32_t
{
	cpuid,
	slat_violation,
	unknown,
};
