#pragma once
#include <cstdint>
#include <span>

namespace hook
{
	bool init() noexcept;

	bool install(std::uintptr_t target_va,
	             std::span<const std::uint8_t> pre_asm,
	             std::span<const std::uint8_t> post_asm,
	             bool monitor) noexcept;

	bool remove(std::uintptr_t target_va) noexcept;
}
