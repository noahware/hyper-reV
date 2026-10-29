#pragma once
#include "kernel.hpp"

#include <pe/image.hpp>

#include <cstdint>
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
	};

	img_dump run(std::uintptr_t mod_va, std::uint64_t cr3 = kern::curr_cr3) noexcept;
}
