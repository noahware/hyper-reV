#pragma once
#include "mem.hpp"
#include "map.hpp"
#include "../arch/current.hpp"
#include <crt.hpp>

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

	enum class direction { from_guest, to_guest };

	template <direction Dir>
	inline bool copy_host(const cr3 guest_cr3, const std::uintptr_t guest_va,
	                      void* const host_buf, const std::size_t size) noexcept
	{
		auto remaining = size;
		auto va = guest_va;
		auto* host = static_cast<std::uint8_t*>(host_buf);

		while (remaining > 0)
		{
			const auto gpa = translate_va(guest_cr3, va);
			if (!gpa)
				return false;

			auto* const mapped = map_phys<std::uint8_t*>(gpa);
			const auto page_left = page_size - (va & page_mask);
			const auto chunk = remaining < page_left ? remaining : page_left;

			if constexpr (Dir == direction::from_guest)
				cstd::crt::memcpy(host, mapped, chunk);
			else
				cstd::crt::memcpy(mapped, host, chunk);

			va += chunk;
			host += chunk;
			remaining -= chunk;
		}

		return true;
	}

	inline bool read_virt(const cr3 guest_cr3, const std::uintptr_t guest_va,
	                      void* const host_dest, const std::size_t size) noexcept
	{
		return copy_host<direction::from_guest>(guest_cr3, guest_va, host_dest, size);
	}

	inline bool write_virt(const cr3 guest_cr3, const std::uintptr_t guest_va,
	                       const void* const host_src, const std::size_t size) noexcept
	{
		return copy_host<direction::to_guest>(guest_cr3, guest_va,
			const_cast<void*>(host_src), size);
	}
}
