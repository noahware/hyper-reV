#include "alias.hpp"

#include "../nt/kernel_module.hpp"
#include "../sys/dump.hpp"
#include "../sys/kernel.hpp"

cmd::alias::map cmd::alias::compile()
{
	map out;

	out.emplace("current_cr3", sys::kern::curr_cr3);

	const nt::kernel_modules modules;

	if (!modules)
		return out;

	for (const auto module : modules)
	{
		if (!module)
			continue;

		const std::string module_name{ module.name };

		out.emplace(module_name, module.base);

		const auto dump = sys::dump::run(module.base);

		if (!dump)
			continue;

		for (const auto& exp : dump->exports())
		{
			if (exp.forwarded)
				continue;

			out.emplace(module_name + "!" + std::string(exp.name), module.base + exp.loc.rva());
		}
	}

	return out;
}
