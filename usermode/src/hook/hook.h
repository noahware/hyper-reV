#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace hook
{
	std::uint8_t set_up();
	void clean_up();

	void set_kernel_detour_holder_base(std::uint64_t address);

	std::uint8_t add_kernel_hook(std::uint64_t routine_to_hook_virtual,
	                             std::span<const std::uint8_t> extra_assembled_bytes,
	                             std::span<const std::uint8_t> post_original_assembled_bytes);

	std::uint8_t remove_kernel_hook(std::uint64_t hooked_routine_virtual, std::uint8_t do_list_erase);

	class kernel_hook_info_t
	{
	public:
		using pfn_type = std::uint64_t;

		kernel_hook_info_t() = default;

		explicit kernel_hook_info_t(const pfn_type original_page_pfn, const pfn_type overflow_page_pfn,
		                            void* const mapped_shadow_page,
		                            const std::uint16_t detour_holder_shadow_offset)
			: original_page_pfn_(original_page_pfn),
			  overflow_page_pfn_(overflow_page_pfn),
			  mapped_shadow_page_(reinterpret_cast<std::uint64_t>(mapped_shadow_page)),
			  detour_holder_shadow_offset_(detour_holder_shadow_offset),
			  reserved_(0)
		{
		}

		[[nodiscard]] std::uint64_t original_page_pfn() const;
		void set_original_page_pfn(std::uint64_t original_page_pfn);

		[[nodiscard]] std::uint64_t overflow_page_pfn() const;
		void set_overflow_page_pfn(std::uint64_t overflow_page_pfn);

		[[nodiscard]] void* mapped_shadow_page() const;
		void set_mapped_shadow_page(void* mapped_shadow_page);

		[[nodiscard]] std::uint16_t detour_holder_shadow_offset() const;
		void set_detour_holder_shadow_offset(std::uint64_t detour_holder_shadow_offset);

	protected:
		pfn_type original_page_pfn_ : 36;
		pfn_type overflow_page_pfn_ : 36;
		std::uint64_t mapped_shadow_page_ : 48;
		std::uint64_t detour_holder_shadow_offset_ : 16;
		std::uint64_t reserved_ : 56;
	};
}
