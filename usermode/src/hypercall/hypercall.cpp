#include "hypercall.hpp"
#include <hypercall.hpp>

extern "C" std::uint64_t send_hypercall(hc::info info);

namespace
{
	std::uint64_t send(const hc::type type) noexcept
	{
		hc::info i;

		i.key = hc::allowed_key;
		i.type = hc::type::test;

		return send_hypercall(i);
	}
}

std::uint64_t hc::test() noexcept
{
	return send(type::test);
}
