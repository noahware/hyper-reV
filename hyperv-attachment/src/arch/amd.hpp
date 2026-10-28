#pragma once
#include "arch.hpp"
#include "amd_def.hpp"

struct amd
{
	using slat_pte       = pte_64;
	using slat_pde       = pde_64;
	using slat_pde_2mb   = pde_2mb_64;
	using slat_pdpte     = pdpte_64;
	using slat_pdpte_1gb = pdpte_1gb_64;
	using slat_pml4e     = pml4e_64;

	template <typename T>
		requires requires(const T e) { e.present; e.flags; }
	static bool slat_is_present(const T& entry)
	{
		return entry.present;
	}

	template <typename T>
		requires requires(T e) { e.present; e.write; e.execute_disable; }
	static void slat_set_perm(T& entry, const page_perm perm)
	{
		entry.present         = (perm & page_perm::read) != page_perm::none;
		entry.write           = (perm & page_perm::write) != page_perm::none;
		entry.execute_disable = (perm & page_perm::execute) == page_perm::none;
	}

	template <typename T>
		requires requires(const T e) { e.present; e.write; e.execute_disable; }
	static page_perm slat_perm(const T& entry)
	{
		page_perm perm = page_perm::none;
		if (entry.present)          perm |= page_perm::read;
		if (entry.write)            perm |= page_perm::write;
		if (!entry.execute_disable) perm |= page_perm::execute;
		return perm;
	}

	template <typename T>
		requires requires(const T e) { e.large_page; e.flags; }
	static bool slat_is_large(const T& entry)
	{
		return entry.large_page;
	}

	static void slat_init_split_pte(slat_pte& pte, const slat_pde_2mb& large, std::uint64_t idx)
	{
		pte.flags = 0;
		pte.execute_disable          = large.execute_disable;
		pte.present                  = large.present;
		pte.write                    = large.write;
		pte.supervisor               = large.supervisor;
		pte.page_level_write_through = large.page_level_write_through;
		pte.page_level_cache_disable = large.page_level_cache_disable;
		pte.global                   = large.global;
		pte.pat                      = large.pat;
		pte.protection_key           = large.protection_key;
		pte.accessed                 = large.accessed;
		pte.dirty                    = large.dirty;
		pte.page_frame_number        = (large.page_frame_number << 9) + idx;
	}

	static void slat_init_split_pde(slat_pde_2mb& pde, const slat_pdpte_1gb& large, std::uint64_t idx)
	{
		pde.flags = 0;
		pde.execute_disable          = large.execute_disable;
		pde.present                  = large.present;
		pde.write                    = large.write;
		pde.supervisor               = large.supervisor;
		pde.page_level_write_through = large.page_level_write_through;
		pde.page_level_cache_disable = large.page_level_cache_disable;
		pde.global                   = large.global;
		pde.pat                      = large.pat;
		pde.protection_key           = large.protection_key;
		pde.accessed                 = large.accessed;
		pde.dirty                    = large.dirty;
		pde.page_frame_number        = (large.page_frame_number << 9) + idx;
		pde.large_page               = 1;
	}

	static void slat_set_table_pde(slat_pde& entry, std::uint64_t child_pfn)
	{
		entry.flags = 0;
		entry.present    = 1;
		entry.write      = 1;
		entry.supervisor = 1;
		entry.page_frame_number = child_pfn;
	}

	static void slat_set_table_pdpte(slat_pdpte& entry, std::uint64_t child_pfn)
	{
		entry.flags = 0;
		entry.present    = 1;
		entry.write      = 1;
		entry.supervisor = 1;
		entry.page_frame_number = child_pfn;
	}

	static void slat_init_merge_pde(slat_pde_2mb& large, const slat_pte& pte)
	{
		large.flags = 0;
		large.execute_disable          = pte.execute_disable;
		large.present                  = pte.present;
		large.write                    = pte.write;
		large.supervisor               = pte.supervisor;
		large.page_level_write_through = pte.page_level_write_through;
		large.page_level_cache_disable = pte.page_level_cache_disable;
		large.global                   = pte.global;
		large.pat                      = pte.pat;
		large.protection_key           = pte.protection_key;
		large.page_frame_number        = pte.page_frame_number >> 9;
		large.large_page               = 1;
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
		case SVM_EXIT_REASON_CPUID:        return exit_reason::cpuid;
		case SVM_EXIT_REASON_NPF:          return exit_reason::slat_violation;
		case SVM_EXIT_REASON_PHYSICAL_NMI: return exit_reason::nmi;
		default:                            return exit_reason::unknown;
		}
	}
};

static_assert(arch_backend<amd>);
