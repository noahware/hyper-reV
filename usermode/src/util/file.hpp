#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace util::file
{
	bool write(const std::filesystem::path& path, std::span<const std::uint8_t> data) noexcept;
	std::vector<std::uint8_t> read(const std::filesystem::path& path) noexcept;
}
