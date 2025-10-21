#include "..\pch.h"

extern "C" std::uint64_t launch_raw_hypercall(hypercall_info_t rcx, std::uint64_t rdx, std::uint64_t r8,
                                              std::uint64_t r9);

namespace hyperv::hypercall
{
static bool hypercall_initialized = false;

std::uint64_t make_hypercall(hypercall_type_t call_type, std::uint64_t call_reserved_data, std::uint64_t rdx,
                             std::uint64_t r8, std::uint64_t r9)
{
    hypercall_info_t hypercall_info = {};
    hypercall_info.primary_key = hypercall_primary_key;
    hypercall_info.secondary_key = hypercall_secondary_key;
    hypercall_info.call_type = call_type;
    hypercall_info.call_reserved_data = call_reserved_data;

    return launch_raw_hypercall(hypercall_info, rdx, r8, r9);
}

std::uint64_t read_host_physical_memory(void *destination_buffer, std::uint64_t host_physical_address,
                                        std::uint64_t size)
{
    hypercall_type_t call_type = hypercall_type_t::host_physical_memory_operation;
    std::uint64_t call_data = static_cast<std::uint64_t>(memory_operation_t::read_operation);
    std::uint64_t dest_virtual_address = reinterpret_cast<std::uint64_t>(destination_buffer);

    return make_hypercall(call_type, call_data, host_physical_address, dest_virtual_address, size);
}

std::uint64_t write_host_physical_memory(void *source_buffer, std::uint64_t host_physical_address, std::uint64_t size)
{
    hypercall_type_t call_type = hypercall_type_t::host_physical_memory_operation;
    std::uint64_t call_data = static_cast<std::uint64_t>(memory_operation_t::write_operation);
    std::uint64_t src_virtual_address = reinterpret_cast<std::uint64_t>(source_buffer);

    return make_hypercall(call_type, call_data, host_physical_address, src_virtual_address, size);
}

std::uint64_t read_guest_physical_memory(void *guest_destination_buffer, std::uint64_t guest_source_physical_address,
                                         std::uint64_t size)
{
    hypercall_type_t call_type = hypercall_type_t::guest_physical_memory_operation;
    std::uint64_t call_data = static_cast<std::uint64_t>(memory_operation_t::read_operation);
    std::uint64_t guest_destination_virtual_address = reinterpret_cast<std::uint64_t>(guest_destination_buffer);

    return make_hypercall(call_type, call_data, guest_source_physical_address, guest_destination_virtual_address, size);
}

std::uint64_t write_guest_physical_memory(void *guest_source_buffer, std::uint64_t guest_destination_physical_address,
                                          std::uint64_t size)
{
    hypercall_type_t call_type = hypercall_type_t::guest_physical_memory_operation;
    std::uint64_t call_data = static_cast<std::uint64_t>(memory_operation_t::write_operation);
    std::uint64_t guest_source_virtual_address = reinterpret_cast<std::uint64_t>(guest_source_buffer);

    return make_hypercall(call_type, call_data, guest_destination_physical_address, guest_source_virtual_address, size);
}

std::uint64_t read_guest_virtual_memory(void *guest_destination_buffer, std::uint64_t guest_source_virtual_address,
                                        std::uint64_t source_cr3, std::uint64_t size)
{
    virt_memory_op_hypercall_info_t memory_op_call = {};
    memory_op_call.call_type = hypercall_type_t::guest_virtual_memory_operation;
    memory_op_call.memory_operation = memory_operation_t::read_operation;
    memory_op_call.address_of_page_directory = source_cr3 >> 12;

    hypercall_info_t hypercall_info = {.value = memory_op_call.value};
    std::uint64_t guest_destination_virtual_address = reinterpret_cast<std::uint64_t>(guest_destination_buffer);

    return make_hypercall(hypercall_info.call_type, hypercall_info.call_reserved_data,
                          guest_destination_virtual_address, guest_source_virtual_address, size);
}

std::uint64_t write_guest_virtual_memory(void *guest_source_buffer, std::uint64_t guest_destination_virtual_address,
                                         std::uint64_t destination_cr3, std::uint64_t size)
{
    virt_memory_op_hypercall_info_t memory_op_call = {};
    memory_op_call.call_type = hypercall_type_t::guest_virtual_memory_operation;
    memory_op_call.memory_operation = memory_operation_t::write_operation;
    memory_op_call.address_of_page_directory = destination_cr3 >> 12;

    hypercall_info_t hypercall_info = {.value = memory_op_call.value};
    std::uint64_t guest_source_virtual_address = reinterpret_cast<std::uint64_t>(guest_source_buffer);

    return make_hypercall(hypercall_info.call_type, hypercall_info.call_reserved_data, guest_source_virtual_address,
                          guest_destination_virtual_address, size);
}

std::uint64_t translate_guest_virtual_address(std::uint64_t guest_virtual_address, std::uint64_t guest_cr3)
{
    hypercall_type_t call_type = hypercall_type_t::translate_guest_virtual_address;
    return make_hypercall(call_type, 0, guest_virtual_address, guest_cr3, 0);
}

std::uint64_t read_guest_cr3()
{
    hypercall_type_t call_type = hypercall_type_t::read_guest_cr3;
    return make_hypercall(call_type, 0, 0, 0, 0);
}

std::uint64_t add_slat_code_hook(std::uint64_t target_guest_physical_address,
                                 std::uint64_t shadow_page_guest_physical_address)
{
    hypercall_type_t call_type = hypercall_type_t::add_slat_code_hook;
    return make_hypercall(call_type, 0, target_guest_physical_address, shadow_page_guest_physical_address, 0);
}

std::uint64_t remove_slat_code_hook(std::uint64_t target_guest_physical_address)
{
    hypercall_type_t call_type = hypercall_type_t::remove_slat_code_hook;
    return make_hypercall(call_type, 0, target_guest_physical_address, 0, 0);
}

std::uint64_t hide_guest_physical_page(std::uint64_t target_guest_physical_address)
{
    hypercall_type_t call_type = hypercall_type_t::hide_guest_physical_page;
    return make_hypercall(call_type, 0, target_guest_physical_address, 0, 0);
}

std::uint64_t flush_logs(std::vector<trap_frame_log_t> &logs)
{
    hypercall_type_t call_type = hypercall_type_t::flush_logs;
    return make_hypercall(call_type, 0, reinterpret_cast<std::uint64_t>(logs.data()), logs.size(), 0);
}

std::uint64_t get_heap_free_page_count()
{
    hypercall_type_t call_type = hypercall_type_t::get_heap_free_page_count;
    return make_hypercall(call_type, 0, 0, 0, 0);
}

bool initialize()
{
    if (hypercall_initialized)
    {
        return true;
    }

    std::uint64_t result = read_guest_cr3();
    if (result == 0)
    {
        // @TODO: add console log
        return false;
    }

    hypercall_initialized = true;
    return true;
}
} // namespace hyperv::hypercall