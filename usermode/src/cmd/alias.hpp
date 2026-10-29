#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

namespace cmd::alias
{
	using map = std::unordered_map<std::string, std::uint64_t>;

	// Walks every loaded kernel module, dumps it, and emits:
	//   "current_cr3"              -> sys::kern::curr_cr3
	//   "<module>"                 -> module base
	//   "<module>!<export>"        -> export absolute address
	// Forwarded exports are skipped.
	map compile();
}
