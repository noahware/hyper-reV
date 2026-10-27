#pragma once
#include <ia32.hpp>
#include <intrin.h>
#include <cstdint>
#include <cstddef>

using phys_addr = std::uint64_t;

constexpr std::size_t page_size = 0x1000;
constexpr std::size_t page_mask = page_size - 1;
constexpr std::size_t page_shift = 12;
constexpr std::size_t pte_count = 512;

namespace arch
{
	inline ::cr0 cr0() noexcept
	{
		return ::cr0{ .flags = __readcr0() };
	}

	inline void set_cr0(const ::cr0 val) noexcept
	{
		__writecr0(val.flags);
	}

	inline ::cr0 disable_wp() noexcept
	{
		const ::cr0 orig_val = cr0();

		::cr0 new_val = orig_val;
		new_val.write_protect = 0;

		set_cr0(new_val);

		return orig_val;
	}

	struct wp_guard
	{
		wp_guard()
			:	orig_val(disable_wp()) { }

		~wp_guard()
		{
			set_cr0(orig_val);
		}

		wp_guard(const wp_guard&) = delete;
		wp_guard& operator=(const wp_guard&) = delete;
		wp_guard(wp_guard&&) = delete;
		wp_guard& operator=(wp_guard&&) = delete;

		::cr0 orig_val;
	};

	inline cr3 cr3() noexcept
	{
		return ::cr3{ .flags = __readcr3() };
	}

	inline void set_cr3(const ::cr3 val) noexcept
	{
		__writecr3(val.flags);
	}
}
