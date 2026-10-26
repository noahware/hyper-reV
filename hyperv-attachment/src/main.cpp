#include <boot_info.hpp>

#include "vmexit/vmexit.hpp"

// returns vmexit handler
extern "C" void* entry_point(const boot_info& bi)
{
    vmexit::init(bi);

    return reinterpret_cast<void*>(&vmexit::handler);
}
