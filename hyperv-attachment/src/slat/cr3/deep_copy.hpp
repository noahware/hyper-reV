#pragma once
#include "../../arch/current.hpp"

namespace slat::cr3
{
	void deep_copy(const arch::slat_pml4e* src_pml4, arch::slat_pml4e* dest_pml4) noexcept;
}
