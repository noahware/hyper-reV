#include "hypercall.hpp"
#include "../../arch/current.hpp"
#include "../../mem/guest.hpp"
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

	case type::phys_mem_op:
	{
		const phys_addr src = regs.rdx;
		const phys_addr dest = regs.r8;
		const auto size = regs.r9;

		regs.rax = mem::guest::copy_phys(src, dest, size);

		return true;
	}

	case type::virt_mem_op:
	{
		const ::cr3 src_cr3{ .flags = regs.rdx };
		const virt_addr src_va{ .val = regs.r8 };
		const ::cr3 dest_cr3{ .flags = regs.r9 };
		const virt_addr dest_va{ .val = regs.r10 };
		const auto size = regs.r11;

		regs.rax = mem::guest::copy_virt(src_cr3, src_va,
		                                 dest_cr3, dest_va, size);

		return true;
	}

	case type::translate_va:
	{
		const ::cr3 target_cr3{ .flags = regs.rdx };
		const virt_addr va{ .val = regs.r8 };

		regs.rax = mem::guest::translate_va(target_cr3, va);

		return true;
	}
	}

	return false;
}
