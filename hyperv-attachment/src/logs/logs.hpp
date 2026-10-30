#pragma once
#include "../arch/current.hpp"
#include <log.hpp>

namespace logs
{
	bool init() noexcept;

	void add(const guest_regs& regs) noexcept;

	std::size_t flush(cr3 guest_cr3, std::uintptr_t guest_va,
	                  std::uint16_t count) noexcept;
}
