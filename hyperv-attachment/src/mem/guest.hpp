#pragma once
#include "mem.hpp"
#include "../arch/current.hpp"

namespace mem::guest
{
	template <class T = void*>
	[[nodiscard]] T map_gpa(const cr3 slat_cr3, const std::uintptr_t gpa) noexcept
	{
		const auto hpa = mem::translate_va(slat_cr3, gpa);
		return hpa ? mem::map_phys<T>(hpa) : T{};
	}

	template <class T = void*>
	[[nodiscard]] T map_gpa(const std::uintptr_t gpa) noexcept
	{
		return map_gpa<T>(arch::slat_cr3(), gpa);
	}

	inline phys_addr translate_va(const cr3 guest_cr3, const cr3 slat_cr3,
	                               const std::uintptr_t va) noexcept
	{
		return mem::translate_va(guest_cr3, va, [slat_cr3](std::uintptr_t gpa) -> void* {
			return map_gpa(slat_cr3, gpa);
		});
	}

	inline phys_addr translate_va(const cr3 guest_cr3, const std::uintptr_t va) noexcept
	{
		return translate_va(guest_cr3, arch::slat_cr3(), va);
	}

	inline bool copy_virt(const cr3 slat_cr3,
	                       const cr3 src_cr3, const std::uintptr_t src,
	                       const cr3 dest_cr3, const std::uintptr_t dest,
	                       const std::uint64_t size) noexcept
	{
		return mem::copy_virt(src_cr3, src, dest_cr3, dest, size,
			[slat_cr3](const cr3 c, const std::uintptr_t va) -> void* {
				const auto gpa = translate_va(c, slat_cr3, va);
				return gpa ? map_gpa(slat_cr3, gpa) : nullptr;
			});
	}

	inline bool copy_virt(const cr3 src_cr3, const std::uintptr_t src,
	                       const cr3 dest_cr3, const std::uintptr_t dest,
	                       const std::uint64_t size) noexcept
	{
		return copy_virt(arch::slat_cr3(), src_cr3, src, dest_cr3, dest, size);
	}

	inline bool copy_phys(const std::uintptr_t src, const std::uintptr_t dest,
	                       const std::uint64_t size) noexcept
	{
		return mem::copy_phys(src, dest, size);
	}
}
