#pragma once
#include "cr3/cr3.hpp"

namespace slat
{
	inline void init() noexcept
	{
		cr3::init();
	}

	inline void first_hook() noexcept
	{
		cr3::first_hook();
	}
}
