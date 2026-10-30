#pragma once

// choose whichever arch you want by uncommenting the following 2 lines
// only 1 arch at a time
#define ARCH_INTEL
//#define ARCH_AMD

#if defined(ARCH_INTEL)
	#include "intel.hpp"
	using arch = intel;
#elif defined(ARCH_AMD)
	#include "amd.hpp"
	using arch = amd;
#else
	#error "Define ARCH_INTEL or ARCH_AMD"
#endif
