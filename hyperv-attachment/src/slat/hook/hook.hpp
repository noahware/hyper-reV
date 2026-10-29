#pragma once
#include "../../arch/defs.hpp"

namespace slat::hook
{
	bool add(phys_addr gpa, phys_addr shadow_hpa) noexcept;
	bool remove(phys_addr gpa) noexcept;

	bool hide(phys_addr gpa, std::size_t page_count = 1) noexcept;
}
