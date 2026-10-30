#pragma once
#include "../arch/cpu.hpp"

namespace mem::pt
{
	void init();
	pml4e_64 identity_pml4e();

	// return overwritten pml4e
	pml4e_64 insert_pml4e(::cr3 cr3, pml4e_64 pml4e, std::size_t idx);
	pml4e_64 insert_identity_pml4e(::cr3 cr3, std::size_t idx);

	::cr3 identity_cr3();
	::cr3 load_identity_cr3();

	[[nodiscard]] bool is_va_exec(::cr3 cr3, std::uintptr_t va) noexcept;

	template <class T = void*>
	[[nodiscard]] T map_phys(phys_addr addr) noexcept
	{
		return reinterpret_cast<T>(addr);
	}
}
