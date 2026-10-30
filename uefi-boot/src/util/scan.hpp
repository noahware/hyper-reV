#pragma once
#include <cstdint>
#include <crt.hpp>
#include <string_view.hpp>
#include <type_traits.hpp>
#include <pe/image.hpp>

namespace util
{
	inline void patch_rel(std::uint8_t* const base, const std::uint8_t* const new_target, const std::uint32_t rva_off,
	                      const std::uint32_t rip_off)
	{
		const auto rip = base + rip_off;

		const std::int32_t expected_rva = static_cast<std::int32_t>(new_target - rip);

		cstd::crt::memcpy(base + rva_off, &expected_rva, sizeof(expected_rva));
	}

	template <class T = std::uint8_t*>
	[[nodiscard]] T resolve_rel(const std::uint8_t* const base, const std::uint32_t rva_off, const std::uint32_t rip_off)
	{
		std::int32_t rva;
		cstd::crt::memcpy(&rva, base + rva_off, sizeof(rva));

		const auto rip = reinterpret_cast<std::uintptr_t>(base + rip_off);

		const auto addr = rip + rva;

		if constexpr (cstd::is_pointer_v<T>)
			return reinterpret_cast<T>(addr);
		else
			return static_cast<T>(addr);
	}

	template <class T = std::uint8_t*>
	[[nodiscard]] T scan_rel(pe::image* const img, const cstd::string_view sig, const std::uint32_t rva_off, const std::uint32_t rip_off)
	{
		const auto base = img->sig_scan(sig);

		if (!base)
			return T{ };

		return resolve_rel<T>(base, rva_off, rip_off);
	}

	template <class T = const std::uint8_t*>
	[[nodiscard]] T scan_rel(const pe::image* const img, const cstd::string_view sig, const std::uint32_t rva_off, const std::uint32_t rip_off)
	{
		const auto base = img->sig_scan(sig);

		if (!base)
			return T{ };

		return resolve_rel<T>(base, rva_off, rip_off);
	}
}
