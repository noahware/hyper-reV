#include "amd.hpp"
#include <boot_info.hpp>
#include <array.hpp>
#include <crt.hpp>

namespace
{
	constexpr std::size_t vmcb_gadget_size = 27;

	cstd::array<std::uint8_t, vmcb_gadget_size> vmcb_gadget;

	void setup_vmcb_gadget(const std::uint8_t* const gadget)
	{
		constexpr std::uint32_t gadget_end = 23;

		const auto end_loc = vmcb_gadget.data() + gadget_end;

		cstd::crt::memcpy(vmcb_gadget.data(), gadget, gadget_end);

		if (gadget[25] == 0x08) // requires deref of rax
		{
			constexpr cstd::array<std::uint8_t, 4> ret_bytes = {
				0x48, 0x8B, 0x00, // mov rax, [rax]
				0xC3 // ret
			};

			cstd::crt::memcpy(end_loc, ret_bytes.data(), sizeof(ret_bytes));
		}
		else
		{
			*end_loc = 0xC3; // ret
		}
	}
}

void amd::init(const boot_info& bi)
{
	setup_vmcb_gadget(bi.vmcb_gadget);
}

struct vmcb* amd::vmcb()
{
	using vmcb_gadget_fn = struct vmcb*(*)();
	const auto fn = reinterpret_cast<vmcb_gadget_fn>(vmcb_gadget.data());

	return fn();
}
