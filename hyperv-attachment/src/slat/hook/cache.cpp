#include "cache.hpp"
#include "../../vcpu/vcpu.hpp"
#include "../../arch/current.hpp"
#include "../../intr/nmi.hpp"

void slat::cache::handle() noexcept
{
	auto& cpu = vcpu::curr();

	if (cpu.flush_needed.exchange(false))
		arch::flush_slat_cache();
}

void slat::cache::flush_all() noexcept
{
	arch::flush_slat_cache();

	vcpu::foreach_but_self([](vcpu& cpu, std::size_t)
	{
		cpu.flush_needed.store(true);
	});

	intr::nmi::send_all_but_self();
}
