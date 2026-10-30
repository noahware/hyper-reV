#include <boot_info.hpp>

#include "arch/current.hpp"
#include "logs/logs.hpp"
#include "vmexit/vmexit.hpp"
#include "mem/alloc/page.hpp"
#include "slat/hook/hook.hpp"
#include "vcpu/vcpu.hpp"

extern "C" void* entry_point(const boot_info& bi)
{
    mem::page::init(bi);
    vcpu::init(bi);
    arch::init(bi);
    vmexit::init(bi);
    slat::hook::init(bi);
    logs::init();

    return reinterpret_cast<void*>(&vmexit::detour);
}
