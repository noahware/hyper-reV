#pragma once
#include "arch.hpp"
#include <ia32.hpp>
#include <intrin.h>

template <class T>
concept has_flags = requires(T val) { val.flags; };

struct intel
{
	using slat_pte   = ept_pte;
	using slat_pde   = ept_pde;
	using slat_pdpte = ept_pdpte;
	using slat_pml4e = ept_pml4e;

	static void set_slat_pte_perm(ept_pte& pte, const page_perm perm)
	{
		pte.read_access    = (perm & page_perm::read) != page_perm::none;
		pte.write_access   = (perm & page_perm::write) != page_perm::none;
		pte.execute_access = (perm & page_perm::execute) != page_perm::none;
	}

	static void init(const boot_info&) {}

	static std::uint64_t vmread(std::uint64_t field)
	{
		std::uint64_t val = 0;
		__vmx_vmread(field, &val);
		return val;
	}

	static void vmwrite(std::uint64_t field, std::uint64_t val)
	{
		__vmx_vmwrite(field, val);
	}

	template <has_flags T>
	static T vmread(std::uint64_t field)
	{
		return T{ .flags = static_cast<decltype(T::flags)>(vmread(field)) };
	}

	template <has_flags T>
	static void vmwrite(std::uint64_t field, const T& val)
	{
		vmwrite(field, val.flags);
	}

	static struct guest_regs guest_regs(std::uint64_t a1, std::uint64_t)
	{
		auto regs = **reinterpret_cast<struct guest_regs**>(a1);
		regs.rsp = vmread(VMCS_GUEST_RSP);
		return regs;
	}

	static void set_guest_regs(std::uint64_t a1, std::uint64_t, const struct guest_regs& regs)
	{
		**reinterpret_cast<struct guest_regs**>(a1) = regs;
		vmwrite(VMCS_GUEST_RSP, regs.rsp);
	}

	static std::uint64_t rip()              { return vmread(VMCS_GUEST_RIP); }
	static void set_rip(std::uint64_t val)  { vmwrite(VMCS_GUEST_RIP, val); }

	static void advance_rip()
	{
		set_rip(rip() + vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH));
	}

	static cr3 guest_cr3() { return vmread<cr3>(VMCS_GUEST_CR3); }

	static exit_reason exit_reason()
	{
		switch (vmread(VMCS_EXIT_REASON) & 0xFFFF)
		{
		case VMX_EXIT_REASON_EXECUTE_CPUID:  return exit_reason::cpuid;
		default:                              return exit_reason::unknown;
		}
	}
};

static_assert(arch_backend<intel>);
