#pragma once
#include "../../arch/defs.hpp"

namespace slat::hook
{
	void handle_page_split(phys_addr page_gpa) noexcept;
}
