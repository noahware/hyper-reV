#include "hypercall.hpp"
#include <hypercall.hpp>

bool hc::handle(guest_regs& regs)
{
	const info i{ .val = regs.rcx };

	switch (i.type)
	{
	case type::test:
		return true;
	}

	return false;
}
