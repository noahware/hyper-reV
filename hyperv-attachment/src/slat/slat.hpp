#pragma once
#include "cr3/cr3.hpp"

#include <atomic.hpp>

namespace slat
{
	inline void init() noexcept
	{
		cr3::init();
	}

	inline void first_hook() noexcept
	{
		static cstd::atomic<bool> done;

		if (done.exchange(true))
			return;

		cr3::first_hook();
	}
}
