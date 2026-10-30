#pragma once
#include "../efi/status.hpp"
#include <pe/image.hpp>

namespace winload
{
	efi::status place_hooks(pe::image* img);
}
