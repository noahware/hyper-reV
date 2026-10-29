#pragma once
#include <cstdint>
#include <span>
#include <string_view>

namespace util::file
{
	bool write(std::string_view path, std::span<const std::uint8_t> data) noexcept;
}
