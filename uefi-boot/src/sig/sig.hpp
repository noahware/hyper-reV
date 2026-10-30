#pragma once
#include <optional.hpp>
#include <cstddef>

#include <pe/sig.hpp>
#include <pe/util.hpp>

#include "../util/scan.hpp"

namespace sig
{
	struct rel
	{
		std::int16_t rva_off;
		std::int16_t rip_off;
		bool deref; // if false, won't follow the rel addr
	};

	template <pe::fixed_string Pattern>
	struct info
	{
		cstd::optional<rel> rel;

		template <class T = std::uint8_t*>
		[[nodiscard]] T find(pe::image* const img) const noexcept
		{
			return resolve<T>(img->sig_scan<Pattern>());
		}

		template <class T = const std::uint8_t*>
		[[nodiscard]] T find(const pe::image* const img) const noexcept
		{
			return resolve<T>(img->sig_scan<Pattern, const std::uint8_t*>());
		}

		// range based variant, for when the image headers aren't accessible but a
		// bounded code range (e.g. walked .text) is
		template <class T = std::uint8_t*>
		[[nodiscard]] T find(const std::uint8_t* const begin, const std::uint8_t* const end) const noexcept
		{
			return resolve<T>(pe::image::sig_scan<Pattern>(begin, end));
		}

		template <class T = const std::uint8_t*>
		[[nodiscard]] T addr(const pe::image* const img) const noexcept
		{
			const auto base = img->sig_scan<Pattern, const std::uint8_t*>();

			if (!base)
				return T{ };

			if (rel)
				return util::resolve_rel<T>(base, rel->rva_off, rel->rip_off);

			return reinterpret_cast<T>(const_cast<std::uint8_t*>(base));
		}

	private:
		template <class T>
		[[nodiscard]] T resolve(const std::uint8_t* const base) const noexcept
		{
			if (!base)
				return T{ };

			if (rel && rel->deref)
				return util::resolve_rel<T>(base, rel->rva_off, rel->rip_off);

			return reinterpret_cast<T>(const_cast<std::uint8_t*>(base));
		}
	};

	// ImgpLoadPeImage
	inline constexpr info<"48 89 44 24 ? E8 ? ? ? ? 8B D8 EB ? BB ? ? ? ? 48 8B"> load_pe_img{
		rel{ .rva_off = 6, .rip_off = 10, .deref = true }
	};

	inline constexpr info<"48 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 89 25"> hvloader_launch_hv{ };

	inline constexpr info<"E8 ? ? ? ? E9 ? ? ? ? 74 0D"> intel_vmexit_handler{
		rel{ .rva_off = 1, .rip_off = 5, .deref = false }
	};

	inline constexpr info<"E8 ? ? ? ? 48 89 04 24 E9"> amd_vmexit_handler{
		rel{ .rva_off = 1, .rip_off = 5, .deref = false }
	};

	inline constexpr info<"65 48 8B 04 25 ? ? ? ? 48 8B 88 ? ? ? ? 48 8B 81 ? ? ? ? 48 8B"> amd_vmcb_gadget{ };

	inline constexpr info<"CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC"> code_cave_16{ };

	// mov edx, gs:[curr_cpu_id_gs_off]; lea rcx, [...]; mov r8d, [...]
	inline constexpr info<"65 8B 14 25 ? ? ? ? 48 8D 0D ? ? ? ? 44 8B 05"> curr_cpu_id_gs_off{
		rel{ .rva_off = 4, .rip_off = 0, .deref = false }
	};
}
