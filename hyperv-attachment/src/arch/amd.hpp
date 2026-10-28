#pragma once
#include "arch.hpp"
#include "amd_def.hpp"

struct amd
{
	static void init(const boot_info& bi);
	static struct vmcb* vmcb();

	static struct guest_regs guest_regs(std::uint64_t, std::uint64_t a2)
	{
		auto regs = **reinterpret_cast<struct guest_regs**>(a2);
		regs.rax = vmcb()->save_state.rax;
		regs.rsp = vmcb()->save_state.rsp;
		return regs;
	}

	static void set_guest_regs(std::uint64_t, std::uint64_t a2, const struct guest_regs& regs)
	{
		**reinterpret_cast<struct guest_regs**>(a2) = regs;
		vmcb()->save_state.rax = regs.rax;
		vmcb()->save_state.rsp = regs.rsp;
	}

	static std::uint64_t rip()              { return vmcb()->save_state.rip; }
	static void set_rip(std::uint64_t val)  { vmcb()->save_state.rip = val; }

	static void advance_rip()
	{
		vmcb()->save_state.rip = vmcb()->control.next_rip;
	}

	static cr3 guest_cr3() { return cr3{ .flags = vmcb()->save_state.cr3 }; }

	static exit_reason exit_reason()
	{
		switch (vmcb()->control.vmexit_reason)
		{
		case SVM_EXIT_REASON_CPUID:  return exit_reason::cpuid;
		default:                      return exit_reason::unknown;
		}
	}
};

static_assert(arch_backend<amd>);
