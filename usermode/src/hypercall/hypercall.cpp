#include "hypercall.hpp"
#include <hypercall.hpp>

extern "C" std::uint64_t send_hypercall(hc::info info);

namespace
{
	std::uint64_t send(const hc::type type) noexcept
	{
		hc::info i;

		i.key = hc::allowed_key;
		i.type = type;

		return send_hypercall(i);
	}
}

std::uint64_t hc::test() noexcept
{
	return send(type::test);
}

std::uint64_t hc::curr_cr3() noexcept
{
	return send(type::curr_cr3);
}
