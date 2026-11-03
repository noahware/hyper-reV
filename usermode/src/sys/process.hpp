#pragma once
#include "kernel.hpp"
#include <cstdint>

namespace sys::process
{
	std::uint64_t cr3( std::uint32_t pid ) noexcept;
}
