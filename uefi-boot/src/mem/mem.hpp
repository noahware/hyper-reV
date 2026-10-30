#pragma once
#include "../efi/status.hpp"
#include "page_tables.hpp"
#include "heap.hpp"

namespace mem
{
	efi::status init()
	{
		const efi::status stat = heap::init();

		if (!stat)
			return stat;

		pt::init();

		return efi::success;
	}
}
