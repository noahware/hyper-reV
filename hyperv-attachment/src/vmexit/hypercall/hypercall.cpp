#include "hypercall.hpp"
#include "../../arch/current.hpp"
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

	case type::curr_cr3:
		regs.rax = arch::guest_cr3().flags;

		return true;
	}

	return false;
}
