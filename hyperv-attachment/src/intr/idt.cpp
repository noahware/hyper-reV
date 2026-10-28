#include "idt.hpp"
#include <ia32.hpp>
#include <intrin.h>

extern "C"
{
	std::uint64_t original_nmi_handler = 0;

	void nmi_standalone_entry();
	void nmi_entry();
}

void intr::idt::hook_nmi() noexcept
{
	segment_descriptor_register_64 idtr{};
	__sidt(&idtr);

	if (idtr.base_address == 0)
		return;

	auto* gates = reinterpret_cast<segment_descriptor_interrupt_gate_64*>(idtr.base_address);
	auto* nmi_gate = &gates[2];
	auto new_gate = *nmi_gate;

	auto new_handler = reinterpret_cast<std::uint64_t>(nmi_entry);

	if (new_gate.present == 0)
	{
		constexpr segment_selector sel{ .index = 1 };

		new_gate.segment_selector = sel.flags;
		new_gate.type = SEGMENT_DESCRIPTOR_TYPE_INTERRUPT_GATE;
		new_gate.present = 1;

		new_handler = reinterpret_cast<std::uint64_t>(nmi_standalone_entry);
	}
	else
	{
		original_nmi_handler = nmi_gate->offset_low
			| (static_cast<std::uint64_t>(nmi_gate->offset_middle) << 16)
			| (static_cast<std::uint64_t>(nmi_gate->offset_high) << 32);
	}

	new_gate.offset_low    = new_handler & 0xFFFF;
	new_gate.offset_middle = (new_handler >> 16) & 0xFFFF;
	new_gate.offset_high   = static_cast<std::uint32_t>(new_handler >> 32);

	*nmi_gate = new_gate;
}
