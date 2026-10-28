#pragma once
#include "../../arch/defs.hpp"

namespace slat::hook
{
	void add(phys_addr gpa, phys_addr shadow_hpa) noexcept;
	void remove(phys_addr gpa) noexcept;
}
