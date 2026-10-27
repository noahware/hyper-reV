#pragma once
#include <cstddef>

struct boot_info;

namespace mem::slab
{
	void* alloc(std::size_t size) noexcept;
	void free(void* p) noexcept;
}
