#include "hva.hpp"
#include "../hvloader/hvloader.hpp"
#include "../arch/cpu.hpp"
#include "../efi/boot.hpp"
#include "../mem/heap.hpp"

#include <pe/image.hpp>
#include <boot_info.hpp>

#define HVA_PATH L"\\efi\\microsoft\\boot\\hyperv-attachment.dll"

namespace
{
	std::uintptr_t hva_entry_point = 0;

	cstd::expected<cstd::vector<std::uint8_t>, efi::status> read_and_delete() noexcept
	{
		auto file = efi::boot_partition.open(HVA_PATH);

		if (!file)
			return cstd::unexpected(file.error());

		const auto file_buf = file->read();

		file->delete_();

		return file_buf;
	}

	// returns entry point
	std::uintptr_t alloc_and_map(const pe::image* const file_img)
	{
		const phys_addr base_addr = mem::heap::alloc_page_aligned(file_img->size());
		const auto virt_img = reinterpret_cast<pe::image*>(base_addr);
		
		const std::size_t hdrs_size = file_img->nt_hdrs()->optional_hdr.size_of_headers;

		cstd::crt::memcpy(virt_img->as(), file_img->as(), hdrs_size);

		for (const auto& sec : file_img->sections())
		{
			const auto dest = virt_img->as() + sec.virtual_address;
			const auto src = file_img->as<const std::uint8_t*>() + sec.pointer_to_raw_data;

			cstd::crt::memcpy(dest, src, sec.size_of_raw_data);
		}

		const std::uintptr_t hva_base = hva_identity_map_base() + base_addr;

		const std::uintptr_t delta = hva_base - file_img->base_addr();

		for (const auto reloc : virt_img->relocs())
		{
			if (reloc.type != pe::reloc_type::dir64)
				continue;

			*reinterpret_cast<std::uintptr_t*>(virt_img->as() + reloc.loc.rva()) += delta;
		}

		return hva_base + virt_img->entry_point();
	}
}

efi::status hva::load_from_disk()
{
	const auto file_buf = read_and_delete();

	if (!file_buf)
		return file_buf.error();

	const auto file_img = reinterpret_cast<const pe::image*>(file_buf->data());

	hva_entry_point = alloc_and_map(file_img);

	return efi::success;
}

std::uint8_t* hva::call_entry_point(const boot_info& bi)
{
	using entry_point_fn = std::uint8_t*(*)(const boot_info&);

	const auto ep = reinterpret_cast<entry_point_fn>(hva_entry_point);

	std::uint8_t* const vmexit_detour = ep(bi);

	return vmexit_detour;
}
