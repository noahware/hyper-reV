#pragma once
#include <cstdint>
#include <cstddef>

struct boot_info;

namespace mem::page
{
	void init(const boot_info& bi) noexcept;

	void* alloc(std::size_t n = 1) noexcept;
	void* free(std::size_t n = 1) noexcept;
}
