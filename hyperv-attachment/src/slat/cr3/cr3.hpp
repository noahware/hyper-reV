#pragma once
#include "../../arch/current.hpp"

namespace slat::cr3
{
	inline ::cr3 hyperv{};
	inline ::cr3 shadow{};

	void init() noexcept;
	void init_shadow() noexcept;
}
