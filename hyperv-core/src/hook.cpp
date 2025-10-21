#include "..\pch.h"
#include <hypercall/hypercall_def.h>
#include <unordered_map>
#include <vector>

namespace hyperv::hook
{
static std::unordered_map<std::uint64_t, hook_context> active_hooks;
static bool hook_system_initialized = false;

bool add_hook(const hook_context &context)
{
    if (!hook_system_initialized)
        return false;

    std::uint64_t target_physical = memory::virtual_to_physical(context.target_address, memory::get_current_cr3());
    if (target_physical == 0)
        return false;

    std::uint64_t shadow_physical = memory::virtual_to_physical(context.shadow_page, memory::get_current_cr3());
    if (shadow_physical == 0)
        return false;

    hypercall_type_t call_type;
    switch (context.type)
    {
    case hook_type::code:
        call_type = hypercall_type_t::add_slat_code_hook;
        break;
    case hook_type::read:
    case hook_type::write:
    case hook_type::read_write:
        // For now, treat all memory hooks as code hooks
        // This can be extended later with specific memory hook types
        call_type = hypercall_type_t::add_slat_code_hook;
        break;
    default:
        return false;
    }

    std::uint64_t result = hypercall::add_slat_code_hook(target_physical, shadow_physical);

    if (result != 0)
    {
        // Store the hook context for later removal
        active_hooks[context.target_address] = context;
        return true;
    }

    return false;
}

bool remove_hook(std::uint64_t target_address)
{
    if (!hook_system_initialized)
        return false;

    auto it = active_hooks.find(target_address);
    if (it == active_hooks.end())
        return false;

    std::uint64_t target_physical = memory::virtual_to_physical(target_address, memory::get_current_cr3());
    if (target_physical == 0)
        return false;

    hypercall_type_t call_type = hypercall_type_t::remove_slat_code_hook;
    std::uint64_t result = hypercall::remove_slat_code_hook(target_physical);

    if (result != 0)
    {
        active_hooks.erase(it);
        return true;
    }

    return false;
}

bool initialize_hook_system()
{
    if (hook_system_initialized)
    {
        return true;
    }

    active_hooks.clear();
    hook_system_initialized = true;
    return true;
}

void cleanup_hook_system()
{
    if (!hook_system_initialized)
        return;

    for (const auto &[address, context] : active_hooks)
    {
        remove_hook(address);
    }

    active_hooks.clear();
    hook_system_initialized = false;
}
} // namespace hyperv::hook