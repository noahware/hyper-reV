#pragma once
#include "../../arch/current.hpp"

namespace slat::cr3
{
	arch::slat_pte* find_split_leaf(::cr3 slat_cr3, phys_addr gpa) noexcept;

	bool merge(::cr3 slat_cr3, phys_addr gpa) noexcept;
}
