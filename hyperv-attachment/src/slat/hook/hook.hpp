#pragma once
#include "../../arch/defs.hpp"
#include "hide.hpp"

namespace slat::hook
{
	void init(const boot_info& bi) noexcept;

	bool add(phys_addr gpa, phys_addr shadow_hpa) noexcept;
	bool remove(phys_addr gpa) noexcept;
}
