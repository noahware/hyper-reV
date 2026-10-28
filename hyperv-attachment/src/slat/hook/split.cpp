#include "split.hpp"
#include "info.hpp"
#include "../cr3/cr3.hpp"
#include "../cr3/split.hpp"
#include "../../arch/current.hpp"
#include "../../mem/guest.hpp"

namespace
{
	bool is_gpa_hooked(const phys_addr gpa) noexcept
	{
		const auto page = gpa & ~page_mask;

		for (const auto& entry : slat::hook::list)
		{
			if ((entry.gpa & ~page_mask) == page)
				return true;
		}

		return false;
	}

	bool could_be_split_instruction(const cr3 slat_cr3) noexcept
	{
		constexpr std::uint64_t max_insn_len = 15;
		constexpr std::uint64_t boundary = page_size - max_insn_len;

		const auto rip = arch::rip();
		const virt_addr va{ .val = rip };

		if (va.off < boundary)
			return false;

		const auto guest_cr3 = arch::guest_cr3();

		const auto rip_gpa = mem::guest::translate_va(guest_cr3, slat_cr3, rip);
		if (rip_gpa && is_gpa_hooked(rip_gpa))
			return true;

		const auto next_page_va = (rip & ~static_cast<std::uint64_t>(page_mask)) + page_size;
		const auto next_gpa = mem::guest::translate_va(guest_cr3, slat_cr3, next_page_va);
		if (next_gpa && is_gpa_hooked(next_gpa))
			return true;

		return false;
	}

	void grant_page_execute(const cr3 slat_cr3, const phys_addr gpa) noexcept
	{
		auto* pte = slat::cr3::find_split_leaf(slat_cr3, gpa);
		if (!pte)
			return;

		const auto perm = arch::slat_perm(*pte) | page_perm::execute;
		arch::slat_set_perm(*pte, perm);
	}
}

void slat::hook::handle_page_split(const phys_addr page_gpa) noexcept
{
	if (could_be_split_instruction(cr3::shadow_cr3))
		grant_page_execute(cr3::shadow_cr3, page_gpa);
}
