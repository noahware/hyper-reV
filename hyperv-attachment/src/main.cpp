#include "arch/arch.h"
#include "hypercall/hypercall.h"
#include "hypercall/hypercall_def.h"
#include "memory_manager/memory_manager.h"
#include "memory_manager/heap_manager.h"
#include "slat/slat.h"
#include "logs/logs.h"
#include "structures/trap_frame.h"
#include <ia32-doc/ia32.hpp>
#include <intrin.h>
#include <cstdint>

#include "crt/crt.h"
#include "interrupts/interrupts.h"

typedef std::uint64_t (*vmexit_handler_t)(std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4);

namespace
{
std::uint8_t *original_vmexit_handler = nullptr;
std::uint64_t uefi_boot_physical_base_address = 0;
std::uint64_t uefi_boot_image_size = 0;

std::uint64_t heap_physical_initial_base = 0;
std::uint64_t heap_total_size_to_hide = 0;
} // namespace

void clean_up_uefi_boot_image()
{
    // todo: check if windows has used this reclaimed memory
    std::uint8_t *mapped_uefi_boot_base =
        reinterpret_cast<std::uint8_t *>(memory_manager::map_host_physical(uefi_boot_physical_base_address));

    crt::set_memory(mapped_uefi_boot_base, 0, uefi_boot_image_size);
}

void process_first_vmexit()
{
    static std::uint8_t is_first_vmexit = 1;

    if (is_first_vmexit == 1)
    {
        slat::process_first_vmexit();
        interrupts::set_up();

        clean_up_uefi_boot_image();

        is_first_vmexit = 0;
    }

    static std::uint8_t has_hidden_heap_pages = 0;
    static std::uint64_t vmexit_count = 0;

    if (has_hidden_heap_pages == 0 && 10000 <= ++vmexit_count)
    {
        std::uint64_t heap_physical_address = heap_physical_initial_base;
        std::uint64_t heap_physical_end = heap_physical_initial_base + heap_total_size_to_hide;

        has_hidden_heap_pages = slat::try_hide_heap_pages(heap_physical_address, heap_physical_end);
    }
}

std::uint64_t do_vmexit_premature_return()
{
#ifdef _INTELMACHINE
    return 0;
#else
    return __readgsqword(0);
#endif
}

std::uint64_t vmexit_handler_detour(std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4)
{
    process_first_vmexit();

    std::uint64_t exit_reason = arch::get_vmexit_reason();

    if (arch::is_cpuid(exit_reason) == 1)
    {
#ifdef _INTELMACHINE
        trap_frame_t *trap_frame = *reinterpret_cast<trap_frame_t **>(a1);
#else
        trap_frame_t *trap_frame = *reinterpret_cast<trap_frame_t **>(a2);
#endif

        hypercall_info_t hypercall_info = {.value = trap_frame->rcx};

        if (hypercall_info.primary_key == hypercall_primary_key &&
            hypercall_info.secondary_key == hypercall_secondary_key)
        {
#ifndef _INTELMACHINE
            vmcb_t *const vmcb = arch::get_vmcb();

            trap_frame->rax = vmcb->save_state.rax;
            trap_frame->rsp = vmcb->save_state.rsp;
#else
            __vmx_vmread(VMCS_GUEST_RSP, &trap_frame->rsp);
#endif

            uint64_t result = hypercall::process(hypercall_info, trap_frame);
            trap_frame->rax = result;

#ifndef _INTELMACHINE
            vmcb->save_state.rax = trap_frame->rax;
            vmcb->save_state.rsp = trap_frame->rsp;
#endif

            arch::advance_guest_rip();
            return do_vmexit_premature_return();
        }
    }
    else if (arch::is_slat_violation(exit_reason) == 1 && slat::process_slat_violation() == 1)
    {
        return do_vmexit_premature_return();
    }
    else if (arch::is_non_maskable_interrupt_exit(exit_reason) == 1)
    {
        interrupts::process_nmi();
    }

    return reinterpret_cast<vmexit_handler_t>(original_vmexit_handler)(a1, a2, a3, a4);
}

void entry_point(std::uint8_t **vmexit_handler_detour_out, std::uint8_t *original_vmexit_handler_routine,
                 std::uint64_t heap_physical_base, std::uint64_t heap_physical_usable_base,
                 std::uint64_t heap_total_size, std::uint64_t _uefi_boot_physical_base_address,
                 std::uint32_t _uefi_boot_image_size,
#ifdef _INTELMACHINE
                 std::uint64_t reserved_one)
{
#else
                 std::uint8_t *get_vmcb_gadget)
{
    arch::parse_vmcb_gadget(get_vmcb_gadget);
#endif
    original_vmexit_handler = original_vmexit_handler_routine;
    uefi_boot_physical_base_address = _uefi_boot_physical_base_address;
    uefi_boot_image_size = _uefi_boot_image_size;

    heap_physical_initial_base = heap_physical_base;
    heap_total_size_to_hide = heap_total_size;

    *vmexit_handler_detour_out = reinterpret_cast<std::uint8_t *>(vmexit_handler_detour);

    std::uint64_t heap_physical_end = heap_physical_base + heap_total_size;
    std::uint64_t heap_usable_size = heap_physical_end - heap_physical_usable_base;

    const std::uint64_t mapped_heap_usable_base = memory_manager::map_host_physical(heap_physical_usable_base);
    heap_manager::set_up(mapped_heap_usable_base, heap_usable_size);

    logs::set_up();
    slat::set_up();
}
