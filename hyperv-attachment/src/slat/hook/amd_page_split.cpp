#include "amd_page_split.h"

#ifndef _INTELMACHINE
#include "../cr3/pte.h"

#include "../../structures/virtual_address.h"
#include "../../crt/crt.h"

static void set_page_executability(const cr3 slat_cr3, const virtual_address_t target_guest_address, const std::uint8_t execute_disable)
{
	slat_pte* const pte = slat::get_pte(slat_cr3, target_guest_address, 1);

	if (pte != nullptr)
	{
		pte->execute_disable = execute_disable;
	}
}

static void set_previous_page_executability(const cr3 slat_cr3, const virtual_address_t target_guest_address, const std::uint8_t execute_disable)
{
	const virtual_address_t previous_page_address = { .address = target_guest_address.address - 0x1000 };

	set_page_executability(slat_cr3, previous_page_address, execute_disable);
}

static void set_next_page_executability(const cr3 slat_cr3, const virtual_address_t target_guest_address, const std::uint8_t execute_disable)
{
	const virtual_address_t next_page_address = { .address = target_guest_address.address + 0x1000 };

	set_page_executability(slat_cr3, next_page_address, execute_disable);
}

void slat::hook::fix_split_instructions(const cr3 slat_cr3, const virtual_address_t target_guest_address)
{
	set_previous_page_executability(slat_cr3, target_guest_address, 0);
	set_next_page_executability(slat_cr3, target_guest_address, 0);
}

void slat::hook::unfix_split_instructions(const entry_t* const hook_entry, const cr3 slat_cr3, const virtual_address_t target_guest_address)
{
	const entry_t* const other_hook_entry_in_range = entry_t::find_closest_in_2mb_range(target_guest_address.address >> 12, hook_entry);

	if (other_hook_entry_in_range != nullptr)
	{
		const std::int64_t source_pfn = static_cast<std::int64_t>(hook_entry->original_pfn());
		const std::int64_t other_pfn = static_cast<std::int64_t>(other_hook_entry_in_range->original_pfn());

		const std::int64_t pfn_difference = source_pfn - other_pfn;
		const std::int64_t abs_pfn_difference = crt::abs(pfn_difference);

		const std::uint8_t is_page_nearby = abs_pfn_difference <= 2;

		std::uint8_t has_fixed = 1;

		if (is_page_nearby == 1 && 0 < pfn_difference)
		{
			set_next_page_executability(slat_cr3, target_guest_address, 1);

			has_fixed = 1;
		}
		else if (is_page_nearby == 1) // negative pfn difference
		{
			set_previous_page_executability(slat_cr3, target_guest_address, 1);

			has_fixed = 1;
		}

		if (abs_pfn_difference == 1)
		{
			// current page must be executable for the nearby hook

			set_page_executability(slat_cr3, target_guest_address, 0);
		}

		if (has_fixed == 1)
		{
			return;
		}
	}

	// no nearby hooks, restore both adjacent pages to non-executable
	set_previous_page_executability(slat_cr3, target_guest_address, 1);
	set_next_page_executability(slat_cr3, target_guest_address, 1);
}
#endif
