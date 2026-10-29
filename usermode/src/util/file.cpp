#include "file.hpp"

#include <fstream>
#include <iterator>

bool util::file::write(const std::filesystem::path& path, const std::span<const std::uint8_t> data) noexcept
{
	std::ofstream out(path, std::ios::binary);

	if (!out)
		return false;

	out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

	return out.good();
}

std::vector<std::uint8_t> util::file::read(const std::filesystem::path& path) noexcept
{
	std::ifstream in(path, std::ios::binary);

	if (!in)
		return {};

	return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
}
