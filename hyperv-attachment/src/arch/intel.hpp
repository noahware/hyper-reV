#pragma once
#include "arch.hpp"
#include <ia32.hpp>
#include <intrin.h>

template <class T>
concept has_flags = requires(T val) { val.flags; };

extern "C" void intel_invept_all(invept_type type, const invept_descriptor& descriptor);

struct intel
{
	using slat_pte       = ept_pte;
	using slat_pde       = ept_pde;
	using slat_pde_2mb   = ept_pde_2mb;
	using slat_pdpte     = ept_pdpte;
	using slat_pdpte_1gb = ept_pdpte_1gb;
	using slat_pml4e     = ept_pml4e;

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
		requires requires(const T e) { e.flags; }
	static bool slat_is_large(const T& entry)
	{
		return (entry.flags >> 7) & 1;
	}

	static void slat_init_split_pte(slat_pte& pte, const slat_pde_2mb& large, std::uint64_t idx)
	{
		pte.flags = 0;
		pte.read_access            = large.read_access;
		pte.write_access           = large.write_access;
		pte.execute_access         = large.execute_access;
		pte.memory_type            = large.memory_type;
		pte.ignore_pat             = large.ignore_pat;
		pte.user_mode_execute      = large.user_mode_execute;
		pte.verify_guest_paging    = large.verify_guest_paging;
		pte.paging_write_access    = large.paging_write_access;
		pte.supervisor_shadow_stack = large.supervisor_shadow_stack;
		pte.suppress_ve            = large.suppress_ve;
		pte.accessed               = large.accessed;
		pte.dirty                  = large.dirty;
		pte.page_frame_number      = (large.page_frame_number << 9) + idx;
	}

	static void slat_init_split_pde(slat_pde_2mb& pde, const slat_pdpte_1gb& large, std::uint64_t idx)
	{
		pde.flags = 0;
		pde.read_access            = large.read_access;
		pde.write_access           = large.write_access;
		pde.execute_access         = large.execute_access;
		pde.memory_type            = large.memory_type;
		pde.ignore_pat             = large.ignore_pat;
		pde.user_mode_execute      = large.user_mode_execute;
		pde.verify_guest_paging    = large.verify_guest_paging;
		pde.paging_write_access    = large.paging_write_access;
		pde.supervisor_shadow_stack = large.supervisor_shadow_stack;
		pde.suppress_ve            = large.suppress_ve;
		pde.accessed               = large.accessed;
		pde.dirty                  = large.dirty;
		pde.page_frame_number      = (large.page_frame_number << 9) + idx;
		pde.large_page             = 1;
	}

	static void slat_set_table_pde(slat_pde& entry, std::uint64_t child_pfn)
	{
		slat_pde tmp{};
		tmp.read_access       = 1;
		tmp.write_access      = 1;
		tmp.execute_access    = 1;
		tmp.user_mode_execute = 1;
		tmp.page_frame_number = child_pfn;
		entry.flags = tmp.flags;
	}

	static void slat_set_table_pdpte(slat_pdpte& entry, std::uint64_t child_pfn)
	{
		slat_pdpte tmp{};
		tmp.read_access       = 1;
		tmp.write_access      = 1;
		tmp.execute_access    = 1;
		tmp.user_mode_execute = 1;
		tmp.page_frame_number = child_pfn;
		entry.flags = tmp.flags;
	}

	static void slat_init_merge_pde(slat_pde_2mb& large, const slat_pte& pte)
	{
		slat_pde_2mb tmp{};
		tmp.read_access            = pte.read_access;
		tmp.write_access           = pte.write_access;
		tmp.execute_access         = pte.execute_access;
		tmp.memory_type            = pte.memory_type;
		tmp.ignore_pat             = pte.ignore_pat;
		tmp.user_mode_execute      = pte.user_mode_execute;
		tmp.verify_guest_paging    = pte.verify_guest_paging;
		tmp.paging_write_access    = pte.paging_write_access;
		tmp.supervisor_shadow_stack = pte.supervisor_shadow_stack;
		tmp.suppress_ve            = pte.suppress_ve;
		tmp.page_frame_number      = pte.page_frame_number >> 9;
		tmp.large_page             = 1;
		large.flags = tmp.flags;
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
