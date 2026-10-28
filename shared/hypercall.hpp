#pragma once

namespace hc
{
	inline constexpr std::uint64_t allowed_key = 0x8A3B55C7927;

	enum class type : std::uint64_t
	{
		test,
		curr_cr3,
		phys_mem_op,
		virt_mem_op,
		translate_va
	};

	union info
	{
		std::uint64_t val;

		struct
		{
			type type : 16;
			std::uint64_t key : 48;
		};
	};

	struct params
	{
		info info;
		std::uint64_t args[5];
	};
}
