#include "..\pch.h"

#ifdef __cplusplus
extern "C"
{
#endif
    bool hyperv_initialize()
    {
        return hyperv::initialize();
    }

    void hyperv_shutdown()
    {
        hyperv::shutdown();
    }

    uint64_t hyperv_memory_read_physical(void *destination, uint64_t guest_physical_address, uint64_t size)
    {
        return hyperv::memory::read_physical(destination, guest_physical_address, size);
    }

    uint64_t hyperv_memory_write_physical(const void *source, uint64_t guest_physical_address, uint64_t size)
    {
        return hyperv::memory::write_physical(source, guest_physical_address, size);
    }

    uint64_t hyperv_memory_read_virtual(void *destination, uint64_t virtual_address, uint64_t cr3, uint64_t size)
    {
        return hyperv::memory::read_virtual(destination, virtual_address, cr3, size);
    }

    uint64_t hyperv_memory_write_virtual(const void *source, uint64_t virtual_address, uint64_t cr3, uint64_t size)
    {
        return hyperv::memory::write_virtual(source, virtual_address, cr3, size);
    }

    std::uint64_t hyperv_read_host_physical_memory(void *destination_buffer, std::uint64_t host_physical_address,
                                                   std::uint64_t size)
    {
        return hyperv::memory::read_host_physical(destination_buffer, host_physical_address, size);
    }

    std::uint64_t hyperv_write_host_physical_memory(const void *source_buffer, std::uint64_t host_physical_address,
                                                    std::uint64_t size)
    {
        return hyperv::memory::write_host_physical(source_buffer, host_physical_address, size);
    }

    uint64_t hyperv_translate_virtual_to_physical(uint64_t virtual_address)
    {
        return hyperv::memory::virtual_to_physical(virtual_address, hyperv::memory::get_current_cr3());
    }

    uint64_t hyperv_memory_get_current_cr3()
    {
        return hyperv::memory::get_current_cr3();
    }

    bool hyperv_hook_add_code_hook(uint64_t target_address, uint64_t shadow_page)
    {
        hyperv::hook::hook_context ctx = {};
        ctx.target_address = target_address;
        ctx.shadow_page = shadow_page;
        ctx.type = hyperv::hook::hook_type::code;

        hyperv::set_last_error(hyperv::error_code::success);
        return hyperv::hook::add_hook(ctx);
    }

    bool hyperv_hook_remove_hook(uint64_t target_address)
    {
        hyperv::set_last_error(hyperv::error_code::success);
        return hyperv::hook::remove_hook(target_address);
    }

    int hyperv_get_last_error()
    {
        return static_cast<int>(hyperv::get_last_error());
    }
#ifdef __cplusplus
}
#endif