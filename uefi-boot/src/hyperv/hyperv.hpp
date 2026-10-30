#pragma once
#include "../efi/status.hpp"
#include <cstdint>

namespace hyperv
{
	efi::status place_hooks(void* entry_point);
}
