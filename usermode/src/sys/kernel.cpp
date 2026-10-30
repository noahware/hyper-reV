#include "kernel.hpp"
#include "dump.hpp"
#include "../nt/kernel_module.hpp"

#include <unordered_set>

bool sys::kern::init() noexcept
{
	curr_cr3 = hc::curr_cr3();

	if (curr_cr3 == 0)
		return false;

	parse_modules();
	return true;
}

void sys::kern::parse_modules() noexcept
{
	const nt::kernel_modules current;

	if (!current)
		return;

	std::unordered_set<std::string> seen;

	for (const auto module : current)
	{
		if (!module)
			continue;

		std::string name{ module.name };

		seen.insert(name);

		if (const auto it = modules_list.find(name); it != modules_list.end())
		{
			if (it->second.base == module.base && it->second.size == module.size)
				continue;
		}

		const auto image = dump::run(module.base);

		if (!image)
			continue;

		kernel_module entry{ module.base, module.size, {} };

		for (const auto& exp : image->exports())
		{
			if (exp.forwarded)
				continue;

			entry.exports.emplace(std::string(exp.name), module.base + exp.loc.rva());
		}

		modules_list[std::move(name)] = std::move(entry);
	}

	for (auto it = modules_list.begin(); it != modules_list.end();)
	{
		if (!seen.contains(it->first))
			it = modules_list.erase(it);
		else
			++it;
	}
}
