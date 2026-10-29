#include "file.hpp"

#include <fstream>
#include <string>

bool util::file::write(const std::string_view path, const std::span<const std::uint8_t> data) noexcept
{
	std::ofstream out(std::string{ path }, std::ios::binary);

	if (!out)
		return false;

	out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

	return out.good();
}
