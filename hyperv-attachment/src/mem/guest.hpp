#pragma once
#include "../arch/defs.hpp"
#include <ia32.hpp>

namespace mem::guest
{
	phys_addr translate_va(cr3 cr3, std::uintptr_t va) noexcept;

	bool copy_phys(std::uintptr_t src, std::uintptr_t dest, std::uint64_t size) noexcept;

	bool copy_virt(cr3 src_cr3, std::uintptr_t src,
	               cr3 dest_cr3, std::uintptr_t dest, std::uint64_t size) noexcept;
}
