#pragma once
#include "../../arch/defs.hpp"
#include <list.hpp>

namespace slat::hook
{
	struct info
	{
		phys_addr gpa;
		phys_addr original_hpa;
		phys_addr shadow_hpa;
	};

	inline cstd::single_linked_list<info> list;
}
