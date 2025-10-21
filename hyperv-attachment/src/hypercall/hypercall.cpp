#include "hypercall.h"
#include <hypercall/hypercall_def.h>
#include "../memory_manager/memory_manager.h"
#include "../slat/slat.h"
#include "../arch/arch.h"
#include "../logs/logs.h"
#include "../crt/crt.h"
#include "../memory_manager/heap_manager.h"

constexpr std::uint64_t usermode_address_limit = 0x00007FFFFFFFFFFFULL;
constexpr std::uint64_t max_physical_address = 0x0000FFFFFFFFFFFFULL;

static bool is_usermode_ptr(const void *ptr, std::uint64_t size)
{
    auto addr = reinterpret_cast<std::uint64_t>(ptr);
    if (addr == 0 || addr > usermode_address_limit)
        return false;
    if (size > usermode_address_limit)
        return false;
    if (addr + size < addr)
        return false; // overflow
    if (addr + size - 1 > usermode_address_limit)
        return false;
    return true;
}

static bool is_valid_physical(std::uint64_t phys, std::uint64_t size)
{
    if (phys > max_physical_address)
        return false;
    if (size > max_physical_address)
        return false;
    if (phys + size < phys)
        return false; // overflow
    if (phys + size - 1 > max_physical_address)
        return false;
    return true;
}

std::uint64_t operate_on_guest_physical_memory(trap_frame_t *trap_frame, memory_operation_t operation)
{
    cr3 guest_cr3 = arch::get_guest_cr3();
    cr3 slat_cr3 = slat::get_cr3();

    std::uint64_t guest_buffer_virtual_address = trap_frame->r8;
    std::uint64_t guest_physical_address = trap_frame->rdx;
    std::uint64_t size_left_to_copy = trap_frame->r9;

    // Fix: use size_left_to_copy as the size parameter
    if (!is_usermode_ptr(reinterpret_cast<void *>(guest_buffer_virtual_address), size_left_to_copy) ||
        !is_valid_physical(guest_physical_address, size_left_to_copy) || size_left_to_copy == 0)
    {
        return 0;
    }

    std::uint64_t bytes_copied = 0;

    while (size_left_to_copy != 0)
    {
        std::uint64_t size_left_of_destination_slat_page = UINT64_MAX;
        std::uint64_t size_left_of_source_slat_page = UINT64_MAX;

        std::uint64_t guest_buffer_physical_address = memory_manager::translate_guest_virtual_address(
            guest_cr3, slat_cr3, virtual_address_t{.value = guest_buffer_virtual_address + bytes_copied});
        if (guest_buffer_physical_address == NULL)
        {
            break;
        }

        std::uint64_t host_destination = memory_manager::map_guest_physical(slat_cr3, guest_buffer_physical_address,
                                                                            &size_left_of_destination_slat_page);
        std::uint64_t host_source = memory_manager::map_guest_physical(slat_cr3, guest_physical_address + bytes_copied,
                                                                       &size_left_of_source_slat_page);

        // Fix: On any mapping failure, break and return bytes_copied
        if (host_destination == NULL || host_source == NULL || size_left_of_destination_slat_page == UINT64_MAX ||
            size_left_of_source_slat_page == UINT64_MAX)
        {
            break;
        }

        if (operation == memory_operation_t::write_operation)
        {
            crt::swap(host_source, host_destination);
        }

        std::uint64_t size_left_of_slat_pages =
            crt::min(size_left_of_source_slat_page, size_left_of_destination_slat_page);

        std::uint64_t copy_size = crt::min(size_left_to_copy, size_left_of_slat_pages);

        if (copy_size == 0)
        {
            break;
        }

        crt::copy_memory(reinterpret_cast<void *>(host_destination), reinterpret_cast<const void *>(host_source),
                         copy_size);

        size_left_to_copy -= copy_size;
        bytes_copied += copy_size;
    }

    return bytes_copied;
}

std::uint64_t operate_on_guest_virtual_memory(trap_frame_t *trap_frame, memory_operation_t operation,
                                              std::uint64_t address_of_page_directory)
{
    cr3 guest_source_cr3 = {.address_of_page_directory = address_of_page_directory};
    cr3 guest_destination_cr3 = arch::get_guest_cr3();
    cr3 slat_cr3 = slat::get_cr3();

    std::uint64_t guest_destination_virtual_address = trap_frame->rdx;
    std::uint64_t guest_source_virtual_address = trap_frame->r8;

    std::uint64_t size_left_to_read = trap_frame->r9;

    std::uint64_t bytes_copied = 0;

    while (size_left_to_read != 0)
    {
        std::uint64_t size_left_of_destination_virtual_page = UINT64_MAX;
        std::uint64_t size_left_of_destination_slat_page = UINT64_MAX;

        std::uint64_t size_left_of_source_virtual_page = UINT64_MAX;
        std::uint64_t size_left_of_source_slat_page = UINT64_MAX;

        std::uint64_t guest_source_physical_address = memory_manager::translate_guest_virtual_address(
            guest_source_cr3, slat_cr3, virtual_address_t{.value = guest_source_virtual_address + bytes_copied},
            &size_left_of_source_virtual_page);
        std::uint64_t guest_destination_physical_address = memory_manager::translate_guest_virtual_address(
            guest_destination_cr3, slat_cr3,
            virtual_address_t{.value = guest_destination_virtual_address + bytes_copied},
            &size_left_of_destination_virtual_page);

        if (size_left_of_destination_virtual_page == UINT64_MAX || size_left_of_source_virtual_page == UINT64_MAX)
        {
            break;
        }

        std::uint64_t host_destination = memory_manager::map_guest_physical(
            slat_cr3, guest_destination_physical_address, &size_left_of_destination_slat_page);
        std::uint64_t host_source =
            memory_manager::map_guest_physical(slat_cr3, guest_source_physical_address, &size_left_of_source_slat_page);

        if (size_left_of_destination_slat_page == UINT64_MAX || size_left_of_source_slat_page == UINT64_MAX)
        {
            break;
        }

        if (operation == memory_operation_t::write_operation)
        {
            crt::swap(host_source, host_destination);
        }

        std::uint64_t size_left_of_slat_pages =
            crt::min(size_left_of_source_slat_page, size_left_of_destination_slat_page);
        std::uint64_t size_left_of_virtual_pages =
            crt::min(size_left_of_source_virtual_page, size_left_of_destination_virtual_page);

        std::uint64_t size_left_of_pages = crt::min(size_left_of_slat_pages, size_left_of_virtual_pages);

        std::uint64_t copy_size = crt::min(size_left_to_read, size_left_of_pages);

        if (copy_size == 0)
        {
            break;
        }

        crt::copy_memory(reinterpret_cast<void *>(host_destination), reinterpret_cast<const void *>(host_source),
                         copy_size);

        size_left_to_read -= copy_size;
        bytes_copied += copy_size;
    }

    return bytes_copied;
}

std::uint8_t copy_stack_data_from_log_exit(std::uint64_t *stack_data, std::uint64_t stack_data_count, cr3 guest_cr3,
                                           std::uint64_t rsp)
{
    if (rsp == 0)
    {
        return 0;
    }

    cr3 slat_cr3 = slat::get_cr3();

    std::uint64_t bytes_read = 0;
    std::uint64_t bytes_remaining = stack_data_count * sizeof(std::uint64_t);

    while (bytes_remaining != 0)
    {
        std::uint64_t virtual_size_left = 0;

        std::uint64_t rsp_guest_physical_address = memory_manager::translate_guest_virtual_address(
            guest_cr3, slat_cr3, virtual_address_t{.value = rsp + bytes_read}, &virtual_size_left);

        if (rsp_guest_physical_address == 0)
        {
            return 0;
        }

        std::uint64_t physical_size_left = 0;

        // rcx has just been pushed onto stack
        std::uint64_t *rsp_mapped = reinterpret_cast<std::uint64_t *>(
            memory_manager::map_guest_physical(slat_cr3, rsp_guest_physical_address, &physical_size_left));

        std::uint64_t size_left_of_page = crt::min(physical_size_left, virtual_size_left);
        std::uint64_t size_to_read = crt::min(bytes_remaining, size_left_of_page);

        if (size_to_read == 0)
        {
            return 0;
        }

        crt::copy_memory(reinterpret_cast<std::uint8_t *>(stack_data) + bytes_read,
                         reinterpret_cast<std::uint8_t *>(rsp_mapped) + bytes_read, size_to_read);

        bytes_remaining -= size_to_read;
        bytes_read += size_to_read;
    }

    return 1;
}

void do_stack_data_copy(trap_frame_log_t &trap_frame, cr3 guest_cr3)
{
    constexpr std::uint64_t stack_data_count = trap_frame_log_stack_data_count + 1;

    std::uint64_t stack_data[stack_data_count] = {};

    copy_stack_data_from_log_exit(&stack_data[0], stack_data_count, guest_cr3, trap_frame.rsp);

    crt::copy_memory(&trap_frame.stack_data, &stack_data[1], sizeof(trap_frame.stack_data));

    trap_frame.rcx = stack_data[0];
    trap_frame.rsp += 8; // get rid of the rcx value we push onto stack ourselves
}

void log_current_state(trap_frame_log_t trap_frame)
{
    cr3 guest_cr3 = arch::get_guest_cr3();

    do_stack_data_copy(trap_frame, guest_cr3);

    trap_frame.cr3 = guest_cr3.flags;
    trap_frame.rip = arch::get_guest_rip();

    logs::add_log(trap_frame);
}

std::uint64_t flush_logs(trap_frame_t *trap_frame)
{
    std::uint64_t stored_logs_count = logs::stored_log_index;

    cr3 guest_cr3 = arch::get_guest_cr3();
    cr3 slat_cr3 = slat::get_cr3();

    std::uint64_t guest_virtual_address = trap_frame->rdx;
    std::uint16_t count = static_cast<std::uint16_t>(trap_frame->r8);

    if (logs::flush(slat_cr3, guest_virtual_address, guest_cr3, count) == 0)
    {
        return -1;
    }

    return stored_logs_count;
}

std::uint64_t operate_on_host_physical_memory(trap_frame_t *trap_frame, memory_operation_t operation)
{
    // Parameter checks
    if (!is_usermode_ptr(reinterpret_cast<void *>(trap_frame->r8), trap_frame->r9) ||
        !is_valid_physical(trap_frame->rdx, trap_frame->r9) || trap_frame->r9 == 0)
    {
        return 0;
    }

    cr3 guest_cr3 = arch::get_guest_cr3();
    cr3 slat_cr3 = slat::get_cr3();

    std::uint64_t guest_buffer_virtual_address = trap_frame->r8;
    std::uint64_t host_physical_address = trap_frame->rdx;

    std::uint64_t size_left_to_copy = trap_frame->r9;

    std::uint64_t bytes_copied = 0;

    while (size_left_to_copy != 0)
    {
        std::uint64_t size_left_of_destination_slat_page = UINT64_MAX;
        std::uint64_t size_left_of_source_slat_page = UINT64_MAX;

        std::uint64_t guest_buffer_physical_address = memory_manager::translate_guest_virtual_address(
            guest_cr3, slat_cr3, virtual_address_t{.value = guest_buffer_virtual_address + bytes_copied});

        std::uint64_t host_destination = memory_manager::map_guest_physical(slat_cr3, guest_buffer_physical_address,
                                                                            &size_left_of_destination_slat_page);

        // source is host physical; map to identity region and compute remaining in that host page
        std::uint64_t host_source = memory_manager::map_host_physical(host_physical_address + bytes_copied);
        std::uint64_t page_offset = (host_physical_address + bytes_copied) & 0xFFF;
        size_left_of_source_slat_page = 0x1000 - page_offset;

        if (size_left_of_destination_slat_page == UINT64_MAX || size_left_of_source_slat_page == UINT64_MAX)
        {
            return UINT64_MAX;
        }

        if (operation == memory_operation_t::write_operation)
        {
            crt::swap(host_source, host_destination);
        }

        std::uint64_t size_left_of_slat_pages =
            crt::min(size_left_of_source_slat_page, size_left_of_destination_slat_page);

        std::uint64_t copy_size = crt::min(size_left_to_copy, size_left_of_slat_pages);

        if (copy_size == 0)
        {
            break;
        }

        crt::copy_memory(reinterpret_cast<void *>(host_destination), reinterpret_cast<const void *>(host_source),
                         copy_size);

        size_left_to_copy -= copy_size;
        bytes_copied += copy_size;
    }

    return bytes_copied;
}

uint64_t hypercall::process(hypercall_info_t hypercall_info, trap_frame_t *trap_frame)
{
    switch (hypercall_info.call_type)
    {
    case hypercall_type_t::guest_physical_memory_operation: {
        memory_operation_t memory_operation = static_cast<memory_operation_t>(hypercall_info.call_reserved_data);
        return operate_on_guest_physical_memory(trap_frame, memory_operation);
    }
    case hypercall_type_t::host_physical_memory_operation: {
        memory_operation_t memory_operation = static_cast<memory_operation_t>(hypercall_info.call_reserved_data);

        return operate_on_host_physical_memory(trap_frame, memory_operation);
    }
    case hypercall_type_t::guest_virtual_memory_operation: {
        virt_memory_op_hypercall_info_t virt_memory_op_info = {.value = hypercall_info.value};

        memory_operation_t memory_operation = virt_memory_op_info.memory_operation;
        std::uint64_t address_of_page_directory = virt_memory_op_info.address_of_page_directory;

        return operate_on_guest_virtual_memory(trap_frame, memory_operation, address_of_page_directory);
    }
    case hypercall_type_t::translate_guest_virtual_address: {
        virtual_address_t guest_virtual_address = virtual_address_t{.value = trap_frame->rdx};

        cr3 target_guest_cr3 = {.flags = trap_frame->r8};
        cr3 slat_cr3 = slat::get_cr3();

        return memory_manager::translate_guest_virtual_address(target_guest_cr3, slat_cr3, guest_virtual_address);
    }
    case hypercall_type_t::read_guest_cr3: {
        cr3 guest_cr3 = arch::get_guest_cr3();
        return guest_cr3.flags;
    }
    case hypercall_type_t::add_slat_code_hook: {
        virtual_address_t target_guest_physical_address = virtual_address_t{.value = trap_frame->rdx};
        virtual_address_t shadow_page_guest_physical_address = virtual_address_t{.value = trap_frame->r8};

        cr3 slat_cr3 = slat::get_cr3();

        return slat::add_slat_code_hook(slat_cr3, target_guest_physical_address, shadow_page_guest_physical_address);
    }
    case hypercall_type_t::remove_slat_code_hook: {
        virtual_address_t target_guest_physical_address = virtual_address_t{.value = trap_frame->rdx};

        cr3 slat_cr3 = slat::get_cr3();

        return slat::remove_slat_code_hook(slat_cr3, target_guest_physical_address);
    }
    case hypercall_type_t::hide_guest_physical_page: {
        virtual_address_t target_guest_physical_address = virtual_address_t{.value = trap_frame->rdx};

        cr3 slat_cr3 = slat::get_cr3();

        return slat::hide_physical_page_from_guest(slat_cr3, target_guest_physical_address);
    }
    case hypercall_type_t::log_current_state: {
        trap_frame_log_t trap_frame_log = {};
        crt::copy_memory(&trap_frame_log, trap_frame, sizeof(trap_frame_t));
        log_current_state(trap_frame_log);
    }
    case hypercall_type_t::flush_logs: {
        return flush_logs(trap_frame);
    }
    case hypercall_type_t::get_heap_free_page_count: {
        return heap_manager::get_free_page_count();
    }
    default:
        break;
    }

    return 0;
}
