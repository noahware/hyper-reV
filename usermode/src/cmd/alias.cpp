#include "alias.hpp"
#include "../sys/kernel.hpp"

cmd::alias::map cmd::alias::compile()
{
	map out;

	out.emplace("current_cr3", sys::kern::curr_cr3);

	for (const auto& [name, info] : sys::kern::modules_list)
	{
		out.emplace(name, info.base);

		for (const auto& [export_name, address] : info.exports)
			out.emplace(name + "!" + export_name, address);
	}

	return out;
}
