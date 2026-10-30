#pragma once
#include "../efi/status.hpp"
#include "../arch/cpu.hpp"
#include <cstddef>

namespace mem::heap
{
	efi::status init() noexcept;

	phys_addr alloc_page() noexcept;
	phys_addr alloc_pages(std::size_t count) noexcept;
	phys_addr alloc_page_aligned(std::size_t size) noexcept;

	phys_addr base() noexcept;
	std::size_t size() noexcept;
	phys_addr curr() noexcept;
}
