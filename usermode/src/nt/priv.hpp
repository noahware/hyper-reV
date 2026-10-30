#pragma once
#include <cstdint>

namespace nt
{
	constexpr std::uint32_t se_debug_privilege = 20;

	// RAII: enables a privilege on construction, restores previous state on destruction.
	class privilege
	{
	public:
		explicit privilege(std::uint32_t priv) noexcept;
		~privilege() noexcept;

		privilege(const privilege&) = delete;
		privilege& operator=(const privilege&) = delete;

		static privilege debug() noexcept
		{
			return privilege{ se_debug_privilege };
		}

		explicit operator bool() const noexcept
		{
			return acquired_;
		}

	private:
		std::uint32_t priv_;
		std::uint8_t prev_state_ = 0;
		bool acquired_ = false;
	};
}
