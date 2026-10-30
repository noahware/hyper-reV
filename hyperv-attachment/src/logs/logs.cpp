#include "logs.hpp"
#include "../mem/guest.hpp"

#include <mutex.hpp>
#include <vector.hpp>

namespace
{
	cstd::vector<logs::entry> ring;
	cstd::mutex mtx;
}

bool logs::init() noexcept
{
	ring.reserve(ring_capacity);

	return ring.capacity() == ring_capacity;
}

void logs::add(const guest_regs& regs) noexcept
{
	const auto guest_cr3 = arch::guest_cr3();

	entry e{};
	e.regs = regs;
	e.rip = arch::rip();
	e.cr3 = guest_cr3.flags;

	mem::guest::read_virt(guest_cr3, regs.rsp,
	                     e.stack_data, sizeof(e.stack_data));

	cstd::lock_guard lock{ mtx };

	if (ring.size() >= ring_capacity)
		return;

	ring.push_back(e);
}

std::size_t logs::flush(const cr3 guest_cr3, const std::uintptr_t guest_va,
                        const std::uint16_t count) noexcept
{
	cstd::lock_guard lock{ mtx };

	const auto n = count < ring.size() ? count : ring.size();
	if (n == 0)
		return 0;

	if (!mem::guest::write_virt(guest_cr3, guest_va, ring.data(), n * sizeof(entry)))
		return 0;

	ring.clear();
	return n;
}
