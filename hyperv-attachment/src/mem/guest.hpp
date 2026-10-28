#pragma once
#include "../arch/defs.hpp"
#include <ia32.hpp>

namespace mem::guest
{
	phys_addr translate_va(cr3 cr3, virt_addr va) noexcept;

	bool copy_phys(phys_addr src, phys_addr dest, std::uint64_t size) noexcept;

	bool copy_virt(cr3 src_cr3, virt_addr src,
	               cr3 dest_cr3, virt_addr dest, std::uint64_t size) noexcept;
}
