#pragma once
#include <string_view>
#include <string>
#include <vector>

namespace script
{
	std::vector<std::uint8_t> compile(std::string_view script_contents);
	std::vector<std::uint8_t> compile_from_path(const std::string& script_path);
}
