#include "nmi.hpp"
#include "idt.hpp"
#include "../slat/hook/cache.hpp"
#include <apic.h>

namespace
{
	apic::controller* apic_inst = nullptr;
}

void intr::nmi::init() noexcept
{
	apic_inst = apic::controller::create_instance();

#ifdef ARCH_INTEL
	idt::hook_nmi();
#endif
}

void intr::nmi::process() noexcept
{
	slat::cache::handle();
}

void intr::nmi::send_all_but_self() noexcept
{
	apic_inst->send_nmi(apic::icr_destination_shorthand::all_but_self);
}
