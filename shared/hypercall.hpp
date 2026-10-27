#pragma once

namespace hc
{
	inline constexpr std::uint16_t allowed_key = 0xA3B55C7927;

	enum class type : std::uint64_t
	{
		test
	};

	union info
	{
		std::uint64_t val;

		struct
		{
			std::uint64_t key : 48;
			type type : 16;
		};
	};
}
