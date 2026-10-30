#pragma once
#include "kernel.hpp"

#include <pe/image.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace sys::dump
{
	struct img_dump
	{
		std::vector<std::uint8_t> data;

		pe::image* img() noexcept
		{
			return reinterpret_cast<pe::image*>(data.data());
		}

		const pe::image* img() const noexcept
		{
			return reinterpret_cast<const pe::image*>(data.data());
		}

		pe::image* operator->() noexcept
		{
			return img();
		}

		const pe::image* operator->() const noexcept
		{
			return img();
		}

		explicit operator bool() const noexcept
		{
			return !data.empty();
		}

		// Patch section headers so disk-view PE tools accept the mapped-layout dump.
		void fix_sections() noexcept;
	};

	img_dump run(std::uintptr_t mod_va, std::uint64_t cr3 = kern::curr_cr3) noexcept;

	// Dump → fix_sections → write. Returns false if any step fails.
	bool to_disk(std::string_view path, std::uintptr_t mod_va, std::uint64_t cr3 = kern::curr_cr3) noexcept;
}
