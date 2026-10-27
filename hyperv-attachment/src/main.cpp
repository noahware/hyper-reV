#include <boot_info.hpp>

#include "vmexit/vmexit.hpp"
#include "mem/alloc/page.hpp"

// returns vmexit handler
extern "C" void* entry_point(const boot_info& bi)
{
    mem::page::init(bi);
    vmexit::init(bi);

    return reinterpret_cast<void*>(&vmexit::handler);
}
