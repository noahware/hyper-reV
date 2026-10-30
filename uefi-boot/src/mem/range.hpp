#pragma once
#include "page_tables.hpp"
#include "../arch/cpu.hpp"

namespace mem
{
	struct code_range
	{
		std::uint8_t* begin;
		std::uint8_t* end;
	};

	[[nodiscard]] inline code_range find_exec_range(void* const start_addr) noexcept
	{
		const auto cr3 = arch::cr3();

		const auto start_page = reinterpret_cast<std::uintptr_t>(start_addr) & ~page_mask;

		auto page = start_page;

		while (page >= page_size && mem::pt::is_va_exec(cr3, page - page_size))
			page -= page_size;

		const auto begin = page;

		page = start_page;

		while (mem::pt::is_va_exec(cr3, page + page_size))
			page += page_size;

		const auto end = page + page_size;

		return { reinterpret_cast<std::uint8_t*>(begin), reinterpret_cast<std::uint8_t*>(end) };
	}
}
