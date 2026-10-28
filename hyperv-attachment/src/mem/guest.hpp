#pragma once
#include "virt.hpp"

namespace mem::guest
{
	inline phys_addr translate_va(const cr3 cr3, const std::uintptr_t va) noexcept
	{
		return mem::translate_va(cr3, va);
	}

	inline bool copy_phys(const std::uintptr_t src, const std::uintptr_t dest,
	                       const std::uint64_t size) noexcept
	{
		return mem::copy_phys(src, dest, size);
	}

	inline bool copy_virt(const cr3 src_cr3, const std::uintptr_t src,
	                       const cr3 dest_cr3, const std::uintptr_t dest,
	                       const std::uint64_t size) noexcept
	{
		return mem::copy_virt(src_cr3, src, dest_cr3, dest, size);
	}
}
