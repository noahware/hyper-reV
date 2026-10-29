#pragma once
#include "../hypercall/hypercall.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace hook
{
	// byte `i` of `v`, little endian
	constexpr std::uint8_t b(const std::uint64_t v, const unsigned i) noexcept
	{
		return static_cast<std::uint8_t>(v >> (i * 8));
	}

	constexpr std::size_t abs_jmp_size = 14;
	constexpr std::size_t monitor_stub_size = 14;

	// push imm32(lo); mov dword [rsp + 4], imm32(hi); ret
	constexpr std::array<std::uint8_t, abs_jmp_size> encode_abs_jmp(const std::uintptr_t target) noexcept
	{
		const auto lo = static_cast<std::uint32_t>(target);
		const auto hi = static_cast<std::uint32_t>(target >> 32);

		return {
			0x68, b(lo, 0), b(lo, 1), b(lo, 2), b(lo, 3),
			0xC7, 0x44, 0x24, 0x04, b(hi, 0), b(hi, 1), b(hi, 2), b(hi, 3),
			0xC3,
		};
	}

	// push rcx; mov rcx, imm64; cpuid; pop rcx
	//
	// add_log only reads the guest regs, so rcx is the only one the stub has to preserve.
	// it stays on top of the stack across the cpuid, which is where the log reader picks
	// the caller's real rcx back up from.
	inline std::array<std::uint8_t, monitor_stub_size> encode_monitor_stub() noexcept
	{
		hc::info info{};
		info.type = hc::type::add_log;
		info.key = hc::allowed_key;

		const auto v = info.val;

		return {
			0x51,
			0x48, 0xB9, b(v, 0), b(v, 1), b(v, 2), b(v, 3), b(v, 4), b(v, 5), b(v, 6), b(v, 7),
			0x0F, 0xA2,
			0x59,
		};
	}
}
