#include "kernel.hpp"

bool sys::kern::init() noexcept
{
	curr_cr3 = hc::curr_cr3();
	return curr_cr3 != 0;
}
