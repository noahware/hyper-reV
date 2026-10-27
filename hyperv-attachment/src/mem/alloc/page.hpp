#pragma once
#include <cstdint>
#include <cstddef>

struct boot_info;

namespace mem::page
{
	void init(const boot_info& bi) noexcept;

	std::size_t index_of(const void* p) noexcept;

	void* alloc(std::size_t n = 1) noexcept;
	void free(void* p, std::size_t n = 1) noexcept;
}
