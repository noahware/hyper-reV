#include "hypercall.hpp"
#include <hypercall.hpp>

bool hc::handle(guest_regs& regs)
{
	const info i{ .val = regs.rcx };

	if (i.key != allowed_key)
		return false;

	switch (i.type)
	{
	case type::test:
		regs.rax = 0x1337;

		return true;
	}

	return false;
}
