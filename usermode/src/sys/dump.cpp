#include "dump.hpp"
#include "kernel.hpp"
#include "../arch/arch.hpp"

#include <cstring>

sys::dump::img_dump sys::dump::run(const std::uintptr_t mod_va, const std::uint64_t cr3) noexcept
{
	std::vector<std::uint8_t> headers(arch::page_size);

	if (!kern::read(mod_va, std::span<std::uint8_t>(headers), cr3))
		return {};

	const auto* const probe = reinterpret_cast<const pe::image*>(headers.data());

	if (!probe->dos_hdr()->ok() || !probe->nt_hdrs()->ok())
		return {};

	img_dump result{ std::vector<std::uint8_t>(probe->size()) };

	std::memcpy(result.data.data(), headers.data(), arch::page_size);

	for (const auto& section : result->sections())
	{
		if (section.virtual_size == 0 || section.virtual_address == 0)
			continue;

		const auto out = std::span<std::uint8_t>(result.data.data() + section.virtual_address, section.virtual_size);

		kern::read(mod_va + section.virtual_address, out, cr3);
	}

	return result;
}
