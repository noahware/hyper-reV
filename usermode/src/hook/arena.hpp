#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace hook::arena
{
	bool init() noexcept;

	// takes a slot in the shadowed padding and writes `bytes` into it
	std::optional<std::uintptr_t> alloc(std::span<const std::uint8_t> bytes) noexcept;

	void free(std::uintptr_t va, std::size_t size) noexcept;
}
