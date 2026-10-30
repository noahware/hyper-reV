#include "script.hpp"

#include "../util/file.hpp"
#include "../util/log.hpp"

#include <keystone/keystone.h>

#include <cctype>
#include <sstream>
#include <string>

namespace
{
	// limp <reg>, <symbol>  ->  mov <reg>, 0x<hex>
	std::string resolve_line(const std::string& line, const cmd::alias::map& aliases) noexcept
	{
		std::istringstream iss(line);
		std::string tok0, tok1, tok2;
		iss >> tok0 >> tok1 >> tok2;

		for (auto& c : tok0) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		if (tok0 != "limp") return line;

		if (!tok1.empty() && tok1.back() == ',') tok1.pop_back();

		const auto it = aliases.find(tok2);
		if (it == aliases.end())
		{
			LOG_ERR("script: unresolved alias '{}'", tok2);
			return line;
		}

		std::ostringstream oss;
		oss << "mov " << tok1 << ", 0x" << std::hex << it->second;
		return oss.str();
	}
}

std::vector<std::uint8_t> script::compile(const std::string_view src, const cmd::alias::map& aliases) noexcept
{
	std::istringstream in{ std::string(src) };
	std::string resolved;
	std::string line;

	while (std::getline(in, line))
	{
		resolved += resolve_line(line, aliases);
		resolved += '\n';
	}

	ks_engine* ks = nullptr;
	if (ks_open(KS_ARCH_X86, KS_MODE_64, &ks) != KS_ERR_OK)
	{
		LOG_ERR("script: keystone init failed");
		return {};
	}

	ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_INTEL);

	std::vector<std::uint8_t> out;

	unsigned char* buf = nullptr;
	std::size_t size = 0;
	std::size_t count = 0;

	if (ks_asm(ks, resolved.c_str(), 0, &buf, &size, &count) == KS_ERR_OK && buf)
	{
		out.assign(buf, buf + size);
		ks_free(buf);
	}
	else
		LOG_ERR("script: assembly failed");

	ks_close(ks);
	return out;
}

std::vector<std::uint8_t> script::compile_file(const std::filesystem::path& path, const cmd::alias::map& aliases) noexcept
{
	const auto bytes = util::file::read(path);
	if (bytes.empty())
	{
		LOG_ERR("script: empty or missing: {}", path.string());
		return {};
	}

	return compile({ reinterpret_cast<const char*>(bytes.data()), bytes.size() }, aliases);
}
