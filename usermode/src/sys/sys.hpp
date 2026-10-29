#pragma once
#include "kernel.hpp"

namespace sys
{
	inline bool init() noexcept
	{
		return kern::init();
	}
}
