#pragma once
#include "arch.hpp"
#include "amd_def.hpp"

struct amd
{
	using slat_pte   = pte_64;
	using slat_pde   = pde_64;
	using slat_pdpte = pdpte_64;
	using slat_pml4e = pml4e_64;

	template <typename T>
		requires requires(const T e) { e.present; e.flags; }
	static bool slat_is_present(const T& entry)
	{
		return entry.present;
	}

	template <typename T>
		requires requires(T e) { e.present; e.write; e.no_execute; }
	static void slat_set_perm(T& entry, const page_perm perm)
	{
		entry.present    = (perm & page_perm::read) != page_perm::none;
		entry.write      = (perm & page_perm::write) != page_perm::none;
		entry.no_execute = (perm & page_perm::execute) == page_perm::none;
	}

	template <typename T>
		requires requires(const T e) { e.present; e.write; e.no_execute; }
	static page_perm slat_perm(const T& entry)
	{
		page_perm perm = page_perm::none;
		if (entry.present)     perm |= page_perm::read;
		if (entry.write)       perm |= page_perm::write;
		if (!entry.no_execute) perm |= page_perm::execute;
		return perm;
	}

	template <typename T>
		requires requires(const T e) { e.large_page; e.flags; }
	static bool slat_is_large(const T& entry)
	{
		return entry.large_page;
	}

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

	static cr3 slat_cr3()                    { return vmcb()->control.nested_cr3; }
	static void set_slat_cr3(const cr3 cr3)
	{
		vmcb()->control.nested_cr3 = cr3;
		vmcb()->control.clean.nested_paging = 0;
	}
	static void flush_slat_cache()
	{
		vmcb()->control.tlb_control = tlb_control::flush_entire_tlb;
	}

	static struct slat_violation slat_violation()
	{
		const npf_exit_info info{ .flags = vmcb()->control.first_exit_info };

		page_perm access = page_perm::none;
		if (info.present)        access |= page_perm::read;
		if (info.write_access)   access |= page_perm::write;
		if (info.execute_access) access |= page_perm::execute;

		return {
			.gpa = vmcb()->control.second_exit_info,
			.access = access,
			.caused_by_translation = static_cast<bool>(info.present),
		};
	}

	static exit_reason exit_reason()
	{
		switch (vmcb()->control.vmexit_reason)
		{
		case SVM_EXIT_REASON_CPUID:  return exit_reason::cpuid;
		case SVM_EXIT_REASON_NPF:    return exit_reason::slat_violation;
		default:                      return exit_reason::unknown;
		}
	}
};

static_assert(arch_backend<amd>);
