#pragma once
#include "kernel.hpp"
#include "../hook/hook.hpp"
#include "../util/log.hpp"

namespace sys
{
	inline bool init() noexcept
	{
		if (!kern::init())
			return false;

		if (!hook::init())
			LOG_ERR("sys::init: hook subsystem disabled");

		return true;
	}
}
