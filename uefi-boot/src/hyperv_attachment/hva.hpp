#pragma once
#include "../efi/status.hpp"
#include <cstdint>

struct boot_info;

// hva = [H]yper[V][A]ttachment
namespace hva
{
	efi::status load_from_disk();

	// returns vmexit handler detour addr
	std::uint8_t* call_entry_point(const boot_info& bi);
}
