#pragma once
#include <cstdint>
#include <hypercall.hpp>

namespace hc
{
	std::uint64_t test() noexcept;
	std::uint64_t curr_cr3() noexcept;

	bool phys_copy(std::uintptr_t src_phys, std::uintptr_t dest_phys,
	               std::uint64_t size) noexcept;

	bool virt_copy(std::uint64_t src_cr3, std::uintptr_t src_va,
	               std::uint64_t dest_cr3, std::uintptr_t dest_va,
	               std::uint64_t size) noexcept;

	std::uintptr_t translate_va(std::uint64_t cr3, std::uintptr_t va) noexcept;
}
