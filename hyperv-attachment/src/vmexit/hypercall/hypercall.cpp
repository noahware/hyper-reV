#include "hypercall.hpp"
#include "../../arch/current.hpp"
#include "../../logs/logs.hpp"
#include "../../mem/alloc/page.hpp"
#include "../../mem/guest.hpp"
#include "../../mem/mem.hpp"
#include "../../slat/cr3/cr3.hpp"
#include "../../slat/hook/hook.hpp"
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
		const auto src_va = static_cast<std::uintptr_t>(regs.r8);
		const ::cr3 dest_cr3{ .flags = regs.r9 };
		const auto dest_va = static_cast<std::uintptr_t>(regs.r10);
		const auto size = regs.r11;

		regs.rax = mem::guest::copy_virt(src_cr3, src_va,
		                                 dest_cr3, dest_va, size);

		return true;
	}

	case type::translate_va:
	{
		const ::cr3 target_cr3{ .flags = regs.rdx };
		const auto va = static_cast<std::uintptr_t>(regs.r8);

		regs.rax = mem::guest::translate_va(target_cr3, va);

		return true;
	}

	case type::hook_add:
	{
		const phys_addr target_gpa = regs.rdx;
		const phys_addr shadow_gpa = regs.r8;

		const auto shadow_hpa = mem::translate_va(slat::cr3::hyperv, shadow_gpa);
		if (!shadow_hpa)
		{
			regs.rax = 0;
			return true;
		}

		regs.rax = slat::hook::add(target_gpa & ~page_mask, shadow_hpa & ~page_mask);

		return true;
	}

	case type::hook_remove:
	{
		const phys_addr target_gpa = regs.rdx;

		regs.rax = slat::hook::remove(target_gpa & ~page_mask);

		return true;
	}

	case type::hide_page:
	{
		const phys_addr target_gpa = regs.rdx;

		regs.rax = slat::hook::hide(target_gpa);

		return true;
	}

	case type::rem_heap_size:
	{
		regs.rax = mem::page::free_count() << page_shift;

		return true;
	}

	case type::add_log:
	{
		logs::add(regs);

		return true;
	}

	case type::flush_logs:
	{
		const auto guest_va = static_cast<std::uintptr_t>(regs.rdx);
		const auto count = static_cast<std::uint16_t>(regs.r8);

		regs.rax = logs::flush(arch::guest_cr3(), guest_va, count);

		return true;
	}
	}

	return false;
}
