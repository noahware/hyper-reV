#pragma once
#include "../../arch/defs.hpp"

#include <cstddef>

struct boot_info;

namespace slat::hook
{
	// takes the ranges that have to be kept out of the guest's sight
	void init_hide(const boot_info& bi) noexcept;

	// points a guest physical range at a zero page, in every root that exists
	bool hide(phys_addr gpa, std::size_t page_count = 1) noexcept;

	bool hide_heap() noexcept;
}
