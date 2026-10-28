#pragma once
#include "arch.hpp"
#include <ia32.hpp>
#include <intrin.h>

template <class T>
concept has_flags = requires(T val) { val.flags; };

extern "C" void intel_invept_all(invept_type type, const invept_descriptor& descriptor);

struct intel
{
	using slat_pte   = ept_pte;
	using slat_pde   = ept_pde;
	using slat_pdpte = ept_pdpte;
	using slat_pml4e = ept_pml4e;

	template <typename T>
		requires requires(const T e) { e.read_access; e.flags; }
	static bool slat_is_present(const T& entry)
	{
		return entry.read_access || entry.write_access || entry.execute_access;
	}

	template <typename T>
		requires requires(T e) { e.read_access; e.write_access; e.execute_access; }
	static void slat_set_perm(T& entry, const page_perm perm)
	{
		entry.read_access    = (perm & page_perm::read) != page_perm::none;
		entry.write_access   = (perm & page_perm::write) != page_perm::none;
		entry.execute_access = (perm & page_perm::execute) != page_perm::none;
	}

	template <typename T>
		requires requires(const T e) { e.read_access; e.write_access; e.execute_access; }
	static page_perm slat_perm(const T& entry)
	{
		page_perm perm = page_perm::none;
		if (entry.read_access)    perm |= page_perm::read;
		if (entry.write_access)   perm |= page_perm::write;
		if (entry.execute_access) perm |= page_perm::execute;
		return perm;
	}

	template <typename T>
		requires requires(const T e) { e.large_page; e.flags; }
	static bool slat_is_large(const T& entry)
	{
		return entry.large_page;
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

	static cr3 slat_cr3()                    { return vmread<cr3>(VMCS_CTRL_EPT_POINTER); }
	static void set_slat_cr3(const cr3 cr3)  { vmwrite(VMCS_CTRL_EPT_POINTER, cr3); }
	static void flush_slat_cache()
	{
		intel_invept_all(invept_type::invept_all_context, {});
	}

	static struct slat_violation slat_violation()
	{
		const vmx_exit_qualification_ept_violation qual{
			.flags = vmread(VMCS_EXIT_QUALIFICATION)
		};

		page_perm access = page_perm::none;
		if (qual.read_access)    access |= page_perm::read;
		if (qual.write_access)   access |= page_perm::write;
		if (qual.execute_access) access |= page_perm::execute;

		return {
			.gpa = vmread(VMCS_GUEST_PHYSICAL_ADDRESS),
			.access = access,
			.caused_by_translation = static_cast<bool>(qual.caused_by_translation),
		};
	}

	static exit_reason exit_reason()
	{
		const auto reason = vmread(VMCS_EXIT_REASON) & 0xFFFF;

		switch (reason)
		{
		case VMX_EXIT_REASON_EXECUTE_CPUID:    return exit_reason::cpuid;
		case VMX_EXIT_REASON_EPT_VIOLATION:    return exit_reason::slat_violation;
		case VMX_EXIT_REASON_EXCEPTION_OR_NMI:
		{
			const auto info = vmread<vmexit_interrupt_information>(VMCS_VMEXIT_INTERRUPTION_INFORMATION);
			if (info.interruption_type == interruption_type::non_maskable_interrupt)
				return exit_reason::nmi;
			return exit_reason::unknown;
		}
		default:                                return exit_reason::unknown;
		}
	}
};

static_assert(arch_backend<intel>);
