#include "virt.hpp"

bool mem::copy_phys(const std::uintptr_t src, const std::uintptr_t dest,
                    const std::uint64_t size) noexcept
{
	auto remaining = size;
	auto curr_src = src;
	auto curr_dest = dest;

	while (remaining > 0)
	{
		const auto src_page_left = page_size - (curr_src & page_mask);
		const auto dest_page_left = page_size - (curr_dest & page_mask);
		const auto chunk = cstd::min(remaining, cstd::min(src_page_left, dest_page_left));

		const auto* src_ptr = mem::map_phys<const std::uint8_t*>(curr_src);
		auto* dest_ptr = mem::map_phys<std::uint8_t*>(curr_dest);

		cstd::crt::memcpy(dest_ptr, src_ptr, chunk);

		curr_src += chunk;
		curr_dest += chunk;
		remaining -= chunk;
	}

	return true;
}
