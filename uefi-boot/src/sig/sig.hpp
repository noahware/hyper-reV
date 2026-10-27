#pragma once
#include <optional.hpp>
#include <cstddef>

#include "../util/scan.hpp"

namespace sig
{
	struct rel
	{
		std::int16_t rva_off;
		std::int16_t rip_off;
		bool deref; // if false, won't follow the rel addr
	};

	template <std::size_t N>
	struct info
	{
		char pattern[N];
		cstd::optional<rel> rel;

		template <class T = std::uint8_t*>
		[[nodiscard]] T find(pe::image* const img) const noexcept
		{
			if (rel && rel->deref)
				return util::scan_rel<T>(img, pattern, rel->rva_off, rel->rip_off);

			return img->sig_scan<T>(pattern);
		}

		template <class T = const std::uint8_t*>
		[[nodiscard]] T find(const pe::image* const img) const noexcept
		{
			if (rel && rel->deref)
				return util::scan_rel<T>(img, pattern, rel->rva_off, rel->rip_off);

			return img->sig_scan<T>(pattern);
		}

		template <class T = const std::uint8_t*>
		[[nodiscard]] T addr(const pe::image* const img) const noexcept
		{
			if (rel)
				return util::scan_rel<T>(img, pattern, rel->rva_off, rel->rip_off);

			return img->sig_scan<T>(pattern);
		}
	};

	// ImgpLoadPeImage
	inline constexpr info load_pe_img{
		"48 89 44 24 ? E8 ? ? ? ? 8B D8 EB ? BB ? ? ? ? 48 8B",
		rel{ .rva_off = 6, .rip_off = 10, .deref = true }
	};

	inline constexpr info hvloader_launch_hv{
		"48 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 89 25"
	};

	inline constexpr info intel_vmexit_handler{
		"E8 ? ? ? ? E9 ? ? ? ? 74 0D",
		rel{ .rva_off = 1, .rip_off = 5, .deref = false }
	};

	inline constexpr info amd_vmexit_handler{
		"E8 ? ? ? ? 48 89 04 24 E9",
		rel{.rva_off = 1, .rip_off = 5, .deref = false }
	};

	inline constexpr info amd_vmcb_gadget{
		"65 48 8B 04 25 ? ? ? ? 48 8B 88 ? ? ? ? 48 8B 81 ? ? ? ? 48 8B"
	};

	inline constexpr info code_cave_16{
		"CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC"
	};

	// mov edx, gs:[curr_cpu_id_gs_off]; lea rcx, [...]; mov r8d, [...]
	inline constexpr info curr_cpu_id_gs_off{
		"65 8B 14 25 ? ? ? ? 48 8D 0D ? ? ? ? 44 8B 05",
		rel{ .rva_off = 4, .rip_off = 0, .deref = false }
	};
}
