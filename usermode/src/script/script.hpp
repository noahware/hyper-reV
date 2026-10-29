#pragma once
#include "../cmd/alias.hpp"

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace script
{
	// assembles intel-syntax x64, resolving `limp <reg>, <symbol>` to `mov <reg>, <address>`
	std::vector<std::uint8_t> compile(std::string_view src, const cmd::alias::map& aliases) noexcept;

	std::vector<std::uint8_t> compile_file(const std::filesystem::path& path, const cmd::alias::map& aliases) noexcept;
}
