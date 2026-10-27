#pragma once

#define ARCH_INTEL

#if defined(ARCH_INTEL)
	#include "intel.hpp"
	using arch = intel;
#elif defined(ARCH_AMD)
	#include "amd.hpp"
	using arch = amd;
#else
	#error "Define ARCH_INTEL or ARCH_AMD"
#endif
