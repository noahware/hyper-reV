#include "hypercall.hpp"

extern "C" std::uint64_t send_hypercall(const hc::params* params);

namespace
{
	std::uint64_t send(const hc::params& p) noexcept
	{
		return send_hypercall(&p);
	}
}

std::uint64_t hc::test() noexcept
{
	params p{};
	p.info.key = allowed_key;
	p.info.type = type::test;

	return send(p);
}

std::uint64_t hc::curr_cr3() noexcept
{
	params p{};
	p.info.key = allowed_key;
	p.info.type = type::curr_cr3;

	return send(p);
}

bool hc::phys_copy(const std::uintptr_t src_phys, const std::uintptr_t dest_phys,
                    const std::uint64_t size) noexcept
{
	params p{};
	p.info.key = allowed_key;
	p.info.type = type::phys_mem_op;
	p.args[0] = src_phys;
	p.args[1] = dest_phys;
	p.args[2] = size;

	return send(p);
}

bool hc::virt_copy(const std::uint64_t src_cr3, const std::uintptr_t src_va,
                    const std::uint64_t dest_cr3, const std::uintptr_t dest_va,
                    const std::uint64_t size) noexcept
{
	params p{};
	p.info.key = allowed_key;
	p.info.type = type::virt_mem_op;
	p.args[0] = src_cr3;
	p.args[1] = src_va;
	p.args[2] = dest_cr3;
	p.args[3] = dest_va;
	p.args[4] = size;

	return send(p);
}

std::uintptr_t hc::translate_va(const std::uint64_t cr3, const std::uintptr_t va) noexcept
{
	params p{};
	p.info.key = allowed_key;
	p.info.type = type::translate_va;
	p.args[0] = cr3;
	p.args[1] = va;

	return send(p);
}
