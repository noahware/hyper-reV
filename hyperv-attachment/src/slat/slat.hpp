#pragma once
#include "cr3/cr3.hpp"

#include <atomic.hpp>

namespace slat
{
	inline void init() noexcept
	{
		cr3::init();
	}
}
