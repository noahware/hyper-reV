#pragma once
#include "../../arch/current.hpp"

namespace slat::cr3
{
	inline ::cr3 hyperv_cr3{};
	inline ::cr3 shadow_cr3{};

	void init() noexcept;
	void first_hook() noexcept;
}
