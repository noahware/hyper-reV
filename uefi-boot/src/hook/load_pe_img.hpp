#pragma once
#include <cstdint>
#include <pe/image.hpp>
#include <string_view.hpp>

struct bl_file_info
{
	std::uint8_t pad[24];
	std::uint16_t* raw_file_name;

	[[nodiscard]] cstd::wstring_view name() const noexcept
	{
		return cstd::wstring_view{ reinterpret_cast<const wchar_t*>(raw_file_name) };
	}
};

using load_pe_img_fn = std::uint64_t(*)(
	bl_file_info* file_info,
	std::int32_t a2,
	pe::image** img,
	std::uint32_t* img_size,
	std::uint64_t* a5,
	std::uint32_t* a6,
	std::uint32_t* a7,
	std::uint64_t a8,
	std::uint64_t a9,
	std::uint64_t a10,
	std::uint64_t a11,
	std::uint64_t a12,
	std::uint64_t a13,
	std::uint64_t a14,
	std::uint64_t a15);
