#include "heap.hpp"

#include <expected.hpp>

#include "../efi/status.hpp"

extern "C" {
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
}

namespace
{
	constexpr std::size_t heap_page_count = 8192;
	phys_addr heap_base = 0;
	phys_addr curr_heap = 0;

	[[nodiscard]] phys_addr heap_end() noexcept
	{
		return heap_base + (heap_page_count * page_size);
	}

	[[nodiscard]] bool inside_heap(const phys_addr addr) noexcept
	{
		return heap_base <= addr && addr < heap_end();
	}

	cstd::expected<phys_addr, efi::status> alloc_phys_pages(const std::size_t count, const EFI_MEMORY_TYPE type) noexcept
	{
		EFI_PHYSICAL_ADDRESS addr = 0;

		const efi::status stat = gBS->AllocatePages(AllocateAnyPages, type, count, &addr);

		if (!stat)
			return cstd::unexpected(stat);

		return static_cast<phys_addr>(addr);
	}
}

efi::status mem::heap::init() noexcept
{
	constexpr EFI_MEMORY_TYPE type = EfiRuntimeServicesData;

	const auto res = alloc_phys_pages(heap_page_count, type);

	if (!res)
		return res.error();

	curr_heap = heap_base = *res;

	return efi::success;
}

phys_addr mem::heap::alloc_page() noexcept
{
	return alloc_pages(1);
}

phys_addr mem::heap::alloc_pages(const std::size_t count) noexcept
{
	const phys_addr addr = curr_heap;

	curr_heap += count * page_size;

	ASSERT(inside_heap(curr_heap));

	return addr;
}

phys_addr mem::heap::alloc_page_aligned(const std::size_t size) noexcept
{
	const std::size_t page_count = (size + page_size - 1) / page_size;

	return alloc_pages(page_count);
}

phys_addr mem::heap::base() noexcept
{
	return heap_base;
}

std::size_t mem::heap::size() noexcept
{
	return heap_page_count * page_size;
}

phys_addr mem::heap::curr() noexcept
{
	return curr_heap;
}
