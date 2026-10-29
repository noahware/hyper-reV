#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace hook::arena
{
	// a slot in the shadowed padding; the caller writes the stub into `bytes`
	struct slot
	{
		std::uintptr_t va = 0;
		std::span<std::uint8_t> bytes;
	};

	bool init() noexcept;

	std::optional<slot> alloc(std::size_t size) noexcept;

	void free(std::uintptr_t va, std::size_t size) noexcept;
}
