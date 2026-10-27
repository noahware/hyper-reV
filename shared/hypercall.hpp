#pragma once

namespace hc
{
	inline constexpr std::uint64_t allowed_key = 0x8A3B55C7927;

	enum class type : std::uint64_t
	{
		test
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
}
