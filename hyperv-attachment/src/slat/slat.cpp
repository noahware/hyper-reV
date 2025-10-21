#include "slat.h"
#include "../arch/arch.h"
#include "../memory_manager/memory_manager.h"
#include "../memory_manager/heap_manager.h"
#include "../crt/crt.h"
#include <ia32-doc/ia32.hpp>
#include <cstdint>

#include "../interrupts/interrupts.h"

#ifdef _INTELMACHINE
#include <intrin.h>

using slat_pml4e = ept_pml4e;
using slat_pdpte_1gb = ept_pdpte_1gb;
using slat_pdpte = ept_pdpte;
using slat_pde_2mb = ept_pde_2mb;
using slat_pde = ept_pde;
using slat_pte = ept_pte;
#else
using slat_pml4e = pml4e_64;
using slat_pdpte_1gb = pdpte_1gb_64;
using slat_pdpte = pdpte_64;
using slat_pde_2mb = pde_2mb_64;
using slat_pde = pde_64;
using slat_pte = pte_64;
#endif

namespace
{
crt::mutex_t hook_mutex = {};

std::uint64_t dummy_page_pfn = 0;

#ifndef _INTELMACHINE
pml4e_64 *hook_nested_pml4 = nullptr;
cr3 hyperv_nested_cr3 = {};
cr3 hook_nested_cr3 = {};
#endif
} // namespace

void slat::set_up()
{
    constexpr std::uint64_t hook_entries_wanted = 0x1000 / sizeof(hook_entry_t);

    void *hook_entries_allocation = heap_manager::allocate_page();

    available_hook_list_head = static_cast<hook_entry_t *>(hook_entries_allocation);

    hook_entry_t *current_entry = available_hook_list_head;

    for (std::uint64_t i = 0; i < hook_entries_wanted - 1; i++)
    {
        current_entry->set_next(current_entry + 1);
        current_entry->set_original_pfn(0);
        current_entry->set_shadow_pfn(0);

        current_entry = current_entry->get_next();
    }

    current_entry->set_original_pfn(0);
    current_entry->set_shadow_pfn(0);
    current_entry->set_next(nullptr);

    void *dummy_page_allocation = heap_manager::allocate_page();

    std::uint64_t dummy_page_physical_address =
        memory_manager::unmap_host_physical(reinterpret_cast<std::uint64_t>(dummy_page_allocation));

    dummy_page_pfn = dummy_page_physical_address >> 12;

    crt::set_memory(dummy_page_allocation, 0, 0x1000);

#ifndef _INTELMACHINE
    hook_nested_pml4 = static_cast<pml4e_64 *>(heap_manager::allocate_page());

    crt::set_memory(hook_nested_pml4, 0, sizeof(pml4e_64) * 512);

    std::uint64_t pml4_physical_address =
        memory_manager::unmap_host_physical(reinterpret_cast<std::uint64_t>(hook_nested_pml4));

    hook_nested_cr3.address_of_page_directory = pml4_physical_address >> 12;
#endif
}

cr3 slat::get_cr3()
{
    cr3 slat_cr3 = {};

#ifdef _INTELMACHINE
    ept_pointer ept_pointer = {};

    __vmx_vmread(VMCS_CTRL_EPT_POINTER, &ept_pointer.flags);

    slat_cr3.address_of_page_directory = ept_pointer.page_frame_number;
#else
    slat_cr3 = hyperv_nested_cr3;
#endif

    return slat_cr3;
}

void slat::flush_current_logical_processor_slat_cache(const std::uint8_t has_slat_cr3_changed)
{
#ifdef _INTELMACHINE
    invalidate_ept_mappings(invept_type::invept_all_context, {});
#else
    vmcb_t *const vmcb = arch::get_vmcb();

    vmcb->control.tlb_control = tlb_control_t::flush_guest_tlb_entries;

    if (has_slat_cr3_changed == 1)
    {
        vmcb->control.clean.nested_paging = 0;
    }
#endif
}

void slat::flush_all_logical_processors_slat_cache()
{
    flush_current_logical_processor_slat_cache();

    interrupts::set_all_nmi_ready();
    interrupts::send_nmi_all_but_self();
}

slat_pml4e *slat_get_pml4e(cr3 slat_cr3, virtual_address_t guest_physical_address)
{
    slat_pml4e *pml4 =
        reinterpret_cast<slat_pml4e *>(memory_manager::map_host_physical(slat_cr3.address_of_page_directory << 12));

    slat_pml4e *pml4e = &pml4[guest_physical_address.pml4_idx];

    return pml4e;
}

slat_pdpte *slat_get_pdpte(slat_pml4e *pml4e, virtual_address_t guest_physical_address)
{
    slat_pdpte *pdpt =
        reinterpret_cast<slat_pdpte *>(memory_manager::map_host_physical(pml4e->page_frame_number << 12));

    slat_pdpte *pdpte = &pdpt[guest_physical_address.pdpt_idx];

    return pdpte;
}

slat_pde *slat_get_pde(slat_pdpte *pdpte, virtual_address_t guest_physical_address)
{
    slat_pde *pd = reinterpret_cast<slat_pde *>(memory_manager::map_host_physical(pdpte->page_frame_number << 12));

    slat_pde *pde = &pd[guest_physical_address.pd_idx];

    return pde;
}

slat_pte *slat_get_pte(slat_pde *pde, virtual_address_t guest_physical_address)
{
    slat_pte *pt = reinterpret_cast<slat_pte *>(memory_manager::map_host_physical(pde->page_frame_number << 12));

    slat_pte *pte = &pt[guest_physical_address.pt_idx];

    return pte;
}

std::uint8_t slat_split_2mb_pde(slat_pde_2mb *large_pde)
{
    slat_pte *pt = static_cast<slat_pte *>(heap_manager::allocate_page());

    if (pt == nullptr)
    {
        return 0;
    }

    for (std::uint64_t i = 0; i < 512; i++)
    {
        slat_pte *pte = &pt[i];

        pte->flags = 0;

#ifdef _INTELMACHINE
        pte->execute_access = large_pde->execute_access;
        pte->read_access = large_pde->read_access;
        pte->write_access = large_pde->write_access;
        pte->memory_type = large_pde->memory_type;
        pte->ignore_pat = large_pde->ignore_pat;
        pte->user_mode_execute = large_pde->user_mode_execute;
        pte->verify_guest_paging = large_pde->verify_guest_paging;
        pte->paging_write_access = large_pde->paging_write_access;
        pte->supervisor_shadow_stack = large_pde->supervisor_shadow_stack;
        pte->suppress_ve = large_pde->suppress_ve;
#else
        pte->execute_disable = large_pde->execute_disable;
        pte->present = large_pde->present;
        pte->write = large_pde->write;
        pte->global = large_pde->global;
        pte->pat = large_pde->pat;
        pte->protection_key = large_pde->protection_key;
        pte->page_level_write_through = large_pde->page_level_write_through;
        pte->page_level_cache_disable = large_pde->page_level_cache_disable;
        pte->supervisor = large_pde->supervisor;
#endif

        pte->accessed = large_pde->accessed;
        pte->dirty = large_pde->dirty;

        pte->page_frame_number = (large_pde->page_frame_number << 9) + i;
    }

    std::uint64_t pt_physical_address = memory_manager::unmap_host_physical(reinterpret_cast<std::uint64_t>(pt));

    slat_pde new_pde = {.flags = 0};

    new_pde.page_frame_number = pt_physical_address >> 12;

#ifdef _INTELMACHINE
    new_pde.read_access = 1;
    new_pde.write_access = 1;
    new_pde.execute_access = 1;
    new_pde.user_mode_execute = 1;
#else
    new_pde.present = 1;
    new_pde.write = 1;
    new_pde.supervisor = 1;
#endif

    large_pde->flags = new_pde.flags;

    return 1;
}

std::uint8_t slat_split_1gb_pdpte(slat_pdpte_1gb *large_pdpte)
{
    slat_pde_2mb *pd = static_cast<slat_pde_2mb *>(heap_manager::allocate_page());

    if (pd == nullptr)
    {
        return 0;
    }

    for (std::uint64_t i = 0; i < 512; i++)
    {
        slat_pde_2mb *pde = &pd[i];

        pde->flags = 0;

#ifdef _INTELMACHINE
        pde->execute_access = large_pdpte->execute_access;
        pde->read_access = large_pdpte->read_access;
        pde->write_access = large_pdpte->write_access;
        pde->memory_type = large_pdpte->memory_type;
        pde->ignore_pat = large_pdpte->ignore_pat;
        pde->user_mode_execute = large_pdpte->user_mode_execute;
        pde->verify_guest_paging = large_pdpte->verify_guest_paging;
        pde->paging_write_access = large_pdpte->paging_write_access;
        pde->supervisor_shadow_stack = large_pdpte->supervisor_shadow_stack;
        pde->suppress_ve = large_pdpte->suppress_ve;
#else
        pde->execute_disable = large_pdpte->execute_disable;
        pde->present = large_pdpte->present;
        pde->write = large_pdpte->write;
        pde->global = large_pdpte->global;
        pde->pat = large_pdpte->pat;
        pde->protection_key = large_pdpte->protection_key;
        pde->page_level_write_through = large_pdpte->page_level_write_through;
        pde->page_level_cache_disable = large_pdpte->page_level_cache_disable;
        pde->supervisor = large_pdpte->supervisor;
#endif

        pde->accessed = large_pdpte->accessed;
        pde->dirty = large_pdpte->dirty;

        pde->page_frame_number = (large_pdpte->page_frame_number << 9) + i;
        pde->large_page = 1;
    }

    std::uint64_t pd_physical_address = memory_manager::unmap_host_physical(reinterpret_cast<std::uint64_t>(pd));

    slat_pdpte new_pdpte = {.flags = 0};

    new_pdpte.page_frame_number = pd_physical_address >> 12;

#ifdef _INTELMACHINE
    new_pdpte.read_access = 1;
    new_pdpte.write_access = 1;
    new_pdpte.execute_access = 1;
    new_pdpte.user_mode_execute = 1;
#else
    new_pdpte.present = 1;
    new_pdpte.write = 1;
    new_pdpte.supervisor = 1;
#endif

    large_pdpte->flags = new_pdpte.flags;

    return 1;
}

slat_pde *slat_get_pde(cr3 slat_cr3, virtual_address_t guest_physical_address, std::uint8_t force_split_pages = 0)
{
    slat_pml4e *pml4e = slat_get_pml4e(slat_cr3, guest_physical_address);

    if (pml4e == nullptr)
    {
        return nullptr;
    }

    slat_pdpte *pdpte = slat_get_pdpte(pml4e, guest_physical_address);

    if (pdpte == nullptr)
    {
        return nullptr;
    }

    slat_pdpte_1gb *large_pdpte = reinterpret_cast<slat_pdpte_1gb *>(pdpte);

    if (large_pdpte->large_page == 1 && (force_split_pages == 0 || slat_split_1gb_pdpte(large_pdpte) == 0))
    {
        return nullptr;
    }

    return slat_get_pde(pdpte, guest_physical_address);
}

slat_pte *slat_get_pte(cr3 slat_cr3, virtual_address_t guest_physical_address, std::uint8_t force_split_pages = 0,
                       std::uint8_t *paging_split_state = nullptr)
{
    slat_pde *pde = slat_get_pde(slat_cr3, guest_physical_address, force_split_pages);

    if (pde == nullptr)
    {
        return nullptr;
    }

    slat_pde_2mb *large_pde = reinterpret_cast<slat_pde_2mb *>(pde);

    if (large_pde->large_page == 1)
    {
        if (force_split_pages == 0 || slat_split_2mb_pde(large_pde) == 0)
        {
            return nullptr;
        }

        if (paging_split_state != nullptr)
        {
            *paging_split_state = 1;
        }
    }

    return slat_get_pte(pde, guest_physical_address);
}

std::uint8_t slat_merge_4kb_pt(cr3 slat_cr3, virtual_address_t guest_physical_address)
{
    slat_pde *pde = slat_get_pde(slat_cr3, guest_physical_address);

    if (pde == nullptr)
    {
        return 0;
    }

    slat_pde_2mb *large_pde = reinterpret_cast<slat_pde_2mb *>(pde);

    if (large_pde->large_page == 1)
    {
        return 1;
    }

    std::uint64_t pt_physical_address = pde->page_frame_number << 12;

    slat_pte *pte = slat_get_pte(pde, guest_physical_address);

    slat_pde_2mb new_large_pde = {large_pde->flags};

#ifdef _INTELMACHINE
    new_large_pde.execute_access = pte->execute_access;
    new_large_pde.read_access = pte->read_access;
    new_large_pde.write_access = pte->write_access;
    new_large_pde.memory_type = pte->memory_type;
    new_large_pde.ignore_pat = pte->ignore_pat;
    new_large_pde.user_mode_execute = pte->user_mode_execute;
    new_large_pde.verify_guest_paging = pte->verify_guest_paging;
    new_large_pde.paging_write_access = pte->paging_write_access;
    new_large_pde.supervisor_shadow_stack = pte->supervisor_shadow_stack;
    new_large_pde.suppress_ve = pte->suppress_ve;
#else
    new_large_pde.execute_disable = pte->execute_disable;
    new_large_pde.present = pte->present;
    new_large_pde.write = pte->write;
    new_large_pde.global = pte->global;
    new_large_pde.pat = pte->pat;
    new_large_pde.protection_key = pte->protection_key;
    new_large_pde.page_level_write_through = pte->page_level_write_through;
    new_large_pde.page_level_cache_disable = pte->page_level_cache_disable;
    new_large_pde.supervisor = pte->supervisor;
#endif

    new_large_pde.page_frame_number = pte->page_frame_number >> 9;
    new_large_pde.large_page = 1;

    *large_pde = new_large_pde;

    void *pt_allocation_mapped = reinterpret_cast<void *>(memory_manager::map_host_physical(pt_physical_address));

    heap_manager::free_page(pt_allocation_mapped);

    return 1;
}

#ifndef _INTELMACHINE
void set_nested_cr3(vmcb_t *const vmcb, cr3 cr3)
{
    vmcb->control.nested_cr3 = cr3;

    slat::flush_current_logical_processor_slat_cache(1);
}

void make_pd_identity_map(pde_2mb_64 *hook_pd, std::uint64_t pml4_index, std::uint64_t pdpt_index)
{
    for (std::uint64_t pd_index = 0; pd_index < 512; pd_index++)
    {
        pde_2mb_64 *hook_pde = &hook_pd[pd_index];

        hook_pde->flags = 0;

        hook_pde->present = 1;
        hook_pde->execute_disable = 1;
        hook_pde->large_page = 1;
        hook_pde->write = 1;

        hook_pde->page_frame_number = (pml4_index << 18) + (pdpt_index << 9) + pd_index;
    }
}

void make_pdpt_identity_map(pdpte_64 *hyperv_pdpt, pdpte_64 *hook_pdpt, std::uint64_t pml4_index)
{
    for (std::uint64_t pdpt_index = 0; pdpt_index < 512; pdpt_index++)
    {
        pdpte_64 *hyperv_pdpte = &hyperv_pdpt[pdpt_index];
        pdpte_64 *hook_pdpte = &hook_pdpt[pdpt_index];

        hook_pdpte->flags = 0;

        if (hyperv_pdpte->present == 0)
        {
            continue;
        }

        pde_64 *hyperv_pd = slat_get_pde(hyperv_pdpte, {});

        if (hyperv_pd == nullptr)
        {
            continue;
        }

        pde_2mb_64 *hook_pd = static_cast<pde_2mb_64 *>(heap_manager::allocate_page());

        if (hook_pd == nullptr)
        {
            continue;
        }

        std::uint64_t hook_pd_physical_address =
            memory_manager::unmap_host_physical(reinterpret_cast<std::uint64_t>(hook_pd));

        hook_pdpte->present = 1;
        hook_pdpte->write = 1;
        hook_pdpte->page_frame_number = hook_pd_physical_address >> 12;

        make_pd_identity_map(hook_pd, pml4_index, pdpt_index);
    }
}

void make_pml4_identity_map(pml4e_64 *hyperv_pml4, pml4e_64 *hook_pml4)
{
    for (std::uint64_t pml4_index = 0; pml4_index < 512; pml4_index++)
    {
        pml4e_64 *hyperv_pml4e = &hyperv_pml4[pml4_index];
        pml4e_64 *hook_pml4e = &hook_pml4[pml4_index];

        hook_pml4e->flags = 0;

        if (hyperv_pml4e->present == 0)
        {
            continue;
        }

        pdpte_64 *hyperv_pdpt = slat_get_pdpte(hyperv_pml4e, {});

        if (hyperv_pdpt == nullptr)
        {
            continue;
        }

        pdpte_64 *hook_pdpt = static_cast<pdpte_64 *>(heap_manager::allocate_page());

        if (hook_pdpt == nullptr)
        {
            continue;
        }

        std::uint64_t hook_pdpt_physical_address =
            memory_manager::unmap_host_physical(reinterpret_cast<std::uint64_t>(hook_pdpt));

        hook_pml4e->present = 1;
        hook_pml4e->write = 1;
        hook_pml4e->page_frame_number = hook_pdpt_physical_address >> 12;

        make_pdpt_identity_map(hyperv_pdpt, hook_pdpt, pml4_index);
    }
}

#endif

void slat::process_first_vmexit()
{
#ifndef _INTELMACHINE
    vmcb_t *const vmcb = arch::get_vmcb();

    hyperv_nested_cr3 = vmcb->control.nested_cr3;

    slat_pml4e *hyperv_pml4 = slat_get_pml4e(hyperv_nested_cr3, {});

    make_pml4_identity_map(hyperv_pml4, hook_nested_pml4);
#endif
}

std::uint8_t slat::try_hide_heap_pages(std::uint64_t heap_physical_address, std::uint64_t heap_physical_end)
{
    cr3 slat_cr3 = get_cr3();

    while (heap_physical_address < heap_physical_end)
    {
        hide_physical_page_from_guest(slat_cr3, virtual_address_t{.value = heap_physical_address});

#ifndef _INTELMACHINE
        hide_physical_page_from_guest(hook_nested_cr3, {.address = heap_physical_address});
#endif

        heap_physical_address += 0x1000;
    }

    return 1;
}

std::uint64_t slat::translate_guest_physical_address(cr3 slat_cr3, virtual_address_t guest_physical_address,
                                                     std::uint64_t *size_left_of_slat_page)
{
#ifdef _INTELMACHINE
    hook_entry_t *hook_entry = hook_entry_t::find(used_hook_list_head, guest_physical_address.value >> 12, nullptr);

    if (hook_entry != nullptr)
    {
        const std::uint64_t page_offset = guest_physical_address.offset_4kb;

        if (size_left_of_slat_page != nullptr)
        {
            *size_left_of_slat_page = (1ull << 12) - page_offset;
        }

        return (hook_entry->get_original_pfn() << 12) + page_offset;
    }
#endif

    return memory_manager::translate_host_virtual_address(slat_cr3, guest_physical_address, size_left_of_slat_page);
}

#ifndef _INTELMACHINE
void set_page_executability(cr3 hook_nested_cr3, virtual_address_t target_guest_address, std::uint8_t execute_disable)
{
    slat_pte *pte = slat_get_pte(hook_nested_cr3, target_guest_address, 1);

    if (pte != nullptr)
    {
        pte->execute_disable = execute_disable;
    }
}

void set_previous_page_executability(cr3 hook_nested_cr3, virtual_address_t target_guest_address,
                                     std::uint8_t execute_disable)
{
    virtual_address_t previous_page_address = {.address = target_guest_address.address - 0x1000};

    set_page_executability(hook_nested_cr3, previous_page_address, execute_disable);
}

void set_next_page_executability(cr3 hook_nested_cr3, virtual_address_t target_guest_address,
                                 std::uint8_t execute_disable)
{
    virtual_address_t next_page_address = {.address = target_guest_address.address + 0x1000};

    set_page_executability(hook_nested_cr3, next_page_address, execute_disable);
}

void fix_split_instructions(cr3 hook_nested_cr3, virtual_address_t target_guest_address)
{
    set_previous_page_executability(hook_nested_cr3, target_guest_address, 0);
    set_next_page_executability(hook_nested_cr3, target_guest_address, 0);
}

void unfix_split_instructions(slat::hook_entry_t *hook_entry, cr3 hook_nested_cr3,
                              virtual_address_t target_guest_address)
{
    slat::hook_entry_t *other_hook_entry_in_range = slat::hook_entry_t::find_closest_in_2mb_range(
        slat::used_hook_list_head, target_guest_address.address >> 12, hook_entry);

    if (other_hook_entry_in_range != nullptr)
    {
        std::int64_t source_pfn = static_cast<std::int64_t>(hook_entry->get_original_pfn());
        std::int64_t other_pfn = static_cast<std::int64_t>(other_hook_entry_in_range->get_original_pfn());

        std::int64_t pfn_difference = source_pfn - other_pfn;
        std::int64_t abs_pfn_difference = crt::abs(pfn_difference);

        std::uint8_t is_page_nearby = abs_pfn_difference <= 2;

        std::uint8_t has_fixed = 1;

        if (is_page_nearby == 1 && 0 < pfn_difference)
        {
            set_next_page_executability(hook_nested_cr3, target_guest_address, 1);

            has_fixed = 1;
        }
        else if (is_page_nearby == 1) // negative pfn difference
        {
            set_previous_page_executability(hook_nested_cr3, target_guest_address, 1);

            has_fixed = 1;
        }

        if (abs_pfn_difference == 1)
        {
            // current page must be executable for the nearby hook

            set_page_executability(hook_nested_cr3, target_guest_address, 0);
        }

        if (has_fixed == 1)
        {
            return;
        }
    }

    // no nearby hooks enough to have to shed executability
    set_previous_page_executability(hook_nested_cr3, target_guest_address, 0);
    set_next_page_executability(hook_nested_cr3, target_guest_address, 0);
}
#endif

std::uint64_t slat::add_slat_code_hook(cr3 slat_cr3, virtual_address_t target_guest_physical_address,
                                       virtual_address_t shadow_page_guest_physical_address)
{
    hook_mutex.lock();

    hook_entry_t *already_present_hook_entry =
        hook_entry_t::find(used_hook_list_head, target_guest_physical_address.value >> 12);

    if (already_present_hook_entry != nullptr)
    {
        hook_mutex.release();

        return 0;
    }

    std::uint8_t paging_split_state = 0;

    slat_pte *target_pte = slat_get_pte(slat_cr3, target_guest_physical_address, 1, &paging_split_state);

    if (target_pte == nullptr)
    {
        hook_mutex.release();

        return 0;
    }

#ifndef _INTELMACHINE
    slat_pte *hook_target_pte = slat_get_pte(hook_nested_cr3, target_guest_physical_address, 1);

    if (hook_target_pte == nullptr)
    {
        hook_mutex.release();

        return 0;
    }
#endif

    if (paging_split_state == 0)
    {
        hook_entry_t *similar_space_hook_entry =
            hook_entry_t::find_in_2mb_range(used_hook_list_head, target_guest_physical_address.value >> 12);

        if (similar_space_hook_entry != nullptr)
        {
            paging_split_state = static_cast<std::uint8_t>(similar_space_hook_entry->get_paging_split_state());
        }
    }

    std::uint64_t shadow_page_host_physical_address =
        translate_guest_physical_address(slat_cr3, shadow_page_guest_physical_address);

    if (shadow_page_host_physical_address == 0)
    {
        hook_mutex.release();

        return 0;
    }

    hook_entry_t *hook_entry = available_hook_list_head;

    if (hook_entry == nullptr)
    {
        hook_mutex.release();

        return 0;
    }

    available_hook_list_head = hook_entry->get_next();

    hook_entry->set_next(used_hook_list_head);
    hook_entry->set_original_pfn(target_pte->page_frame_number);
    hook_entry->set_shadow_pfn(shadow_page_host_physical_address >> 12);
    hook_entry->set_paging_split_state(paging_split_state);

#ifdef _INTELMACHINE
    hook_entry->set_original_read_access(target_pte->read_access);
    hook_entry->set_original_write_access(target_pte->write_access);
    hook_entry->set_original_execute_access(target_pte->execute_access);

    target_pte->page_frame_number = hook_entry->get_shadow_pfn();

    target_pte->execute_access = 1;
    target_pte->read_access = 0;
    target_pte->write_access = 0;
#else
    hook_entry->set_original_execute_access(!target_pte->execute_disable);

    hook_target_pte->execute_disable = 0;
    hook_target_pte->page_frame_number = hook_entry->get_shadow_pfn();

    fix_split_instructions(hook_nested_cr3, target_guest_physical_address);

    target_pte->execute_disable = 1;
#endif

    used_hook_list_head = hook_entry;

    hook_mutex.release();

    flush_all_logical_processors_slat_cache();

    return 1;
}

std::uint8_t does_hook_need_merge(slat::hook_entry_t *hook_entry, virtual_address_t guest_physical_address)
{
    if (hook_entry == nullptr)
    {
        return 0;
    }

    std::uint8_t does_source_hook_require_merge = hook_entry->get_paging_split_state() == 1;

    if (does_source_hook_require_merge == 0)
    {
        return 0;
    }

    slat::hook_entry_t *other_hook_entry_in_range = slat::hook_entry_t::find_in_2mb_range(
        slat::used_hook_list_head, guest_physical_address.value >> 12, hook_entry);

    return other_hook_entry_in_range == nullptr;
}

std::uint8_t clean_up_hook_ptes(cr3 slat_cr3, virtual_address_t target_guest_physical_address,
                                slat::hook_entry_t *hook_entry)
{
    slat_pte *target_pte = slat_get_pte(slat_cr3, target_guest_physical_address);

    if (target_pte == nullptr)
    {
        hook_mutex.release();

        return 0;
    }

#ifndef _INTELMACHINE
    slat_pte *hook_target_pte = slat_get_pte(hook_nested_cr3, target_guest_physical_address);

    if (hook_target_pte == nullptr)
    {
        return 0;
    }
#endif

    slat_pte new_pte = {.flags = target_pte->flags};

#ifdef _INTELMACHINE
    new_pte.page_frame_number = hook_entry->get_original_pfn();

    new_pte.read_access = hook_entry->get_original_read_access();
    new_pte.write_access = hook_entry->get_original_write_access();
    new_pte.execute_access = hook_entry->get_original_execute_access();
#else
    new_pte.execute_disable = !hook_entry->get_original_execute_access();
#endif

    *target_pte = new_pte;

#ifndef _INTELMACHINE
    hook_target_pte->page_frame_number = hook_entry->get_original_pfn();
    hook_target_pte->execute_disable = 1;

    unfix_split_instructions(hook_entry, hook_nested_cr3, target_guest_physical_address);
#endif

    if (does_hook_need_merge(hook_entry, target_guest_physical_address) == 1)
    {
        slat_merge_4kb_pt(slat_cr3, target_guest_physical_address);

#ifndef _INTELMACHINE
        slat_merge_4kb_pt(slat_cr3, target_guest_physical_address);
#endif
    }

    return 1;
}

void clean_up_hook_entry(slat::hook_entry_t *hook_entry, slat::hook_entry_t *previous_hook_entry)
{
    if (previous_hook_entry == nullptr)
    {
        slat::used_hook_list_head = hook_entry->get_next();
    }
    else
    {
        previous_hook_entry->set_next(hook_entry->get_next());
    }

    hook_entry->set_next(slat::available_hook_list_head);

    slat::available_hook_list_head = hook_entry;
}

std::uint64_t slat::remove_slat_code_hook(cr3 slat_cr3, virtual_address_t target_guest_physical_address)
{
    hook_mutex.lock();

    hook_entry_t *previous_hook_entry = nullptr;

    hook_entry_t *hook_entry =
        hook_entry_t::find(used_hook_list_head, target_guest_physical_address.value >> 12, &previous_hook_entry);

    if (hook_entry == nullptr)
    {
        hook_mutex.release();

        return 0;
    }

    std::uint8_t hook_pte_cleanup_status = clean_up_hook_ptes(slat_cr3, target_guest_physical_address, hook_entry);

    clean_up_hook_entry(hook_entry, previous_hook_entry);

    hook_mutex.release();

    flush_all_logical_processors_slat_cache();

    return hook_pte_cleanup_status;
}

std::uint64_t slat::hide_physical_page_from_guest(cr3 slat_cr3, virtual_address_t guest_physical_address)
{
    hook_mutex.lock();

    slat_pte *target_pte = slat_get_pte(slat_cr3, guest_physical_address, 1);

    if (target_pte == nullptr)
    {
        hook_mutex.release();

        return 0;
    }

    target_pte->page_frame_number = dummy_page_pfn;

    hook_mutex.release();

    return 1;
}

std::uint8_t slat::process_slat_violation()
{
#ifdef _INTELMACHINE
    vmx_exit_qualification_ept_violation qualification = {};

    __vmx_vmread(VMCS_EXIT_QUALIFICATION, &qualification.flags);

    if (qualification.execute_access == 1 && (qualification.write_access == 1 || qualification.read_access == 1))
    {
        return 0;
    }

    virtual_address_t physical_address = {};

    __vmx_vmread(qualification.caused_by_translation == 1 ? VMCS_GUEST_PHYSICAL_ADDRESS
                                                          : VMCS_EXIT_GUEST_LINEAR_ADDRESS,
                 &physical_address.value);

    hook_entry_t *hook_entry = hook_entry_t::find(used_hook_list_head, physical_address.value >> 12);

    if (hook_entry == nullptr)
    {
        return 0;
    }

    ept_pte *pte = slat_get_pte(get_cr3(), physical_address);

    if (pte == nullptr)
    {
        return 0;
    }

    if (qualification.execute_access == 1)
    {
        pte->read_access = 0;
        pte->write_access = 0;
        pte->execute_access = 1;

        pte->page_frame_number = hook_entry->get_shadow_pfn();
    }
    else
    {
        pte->read_access = 1;
        pte->write_access = 1;
        pte->execute_access = 0;

        pte->page_frame_number = hook_entry->get_original_pfn();
    }
#else
    vmcb_t *const vmcb = arch::get_vmcb();

    npf_exit_info_1 npf_info = {.flags = vmcb->control.first_exit_info};

    if (npf_info.present == 0 || npf_info.execute_access == 0)
    {
        return 0;
    }

    virtual_address_t physical_address = {vmcb->control.second_exit_info};

    hook_entry_t *hook_entry = hook_entry_t::find(used_hook_list_head, physical_address.address >> 12);

    if (hook_entry == nullptr)
    {
        if (vmcb->control.nested_cr3.flags == hook_nested_cr3.flags)
        {
            set_nested_cr3(vmcb, hyperv_nested_cr3);

            return 1;
        }

        return 0;
    }

    set_nested_cr3(vmcb, hook_nested_cr3);
#endif

    return 1;
}

slat::hook_entry_t *slat::hook_entry_t::get_next() const
{
    return reinterpret_cast<hook_entry_t *>(this->next);
}

void slat::hook_entry_t::set_next(hook_entry_t *next_entry)
{
    this->next = reinterpret_cast<std::uint64_t>(next_entry);
}

std::uint64_t slat::hook_entry_t::get_original_pfn() const
{
    return (static_cast<uint64_t>(this->higher_original_pfn) << 32) | this->lower_original_pfn;
}

std::uint64_t slat::hook_entry_t::get_shadow_pfn() const
{
    return (static_cast<uint64_t>(this->higher_shadow_pfn) << 32) | this->lower_shadow_pfn;
}

std::uint64_t slat::hook_entry_t::get_original_read_access() const
{
    return this->original_read_access;
}

std::uint64_t slat::hook_entry_t::get_original_write_access() const
{
    return this->original_write_access;
}

std::uint64_t slat::hook_entry_t::get_original_execute_access() const
{
    return this->original_execute_access;
}

std::uint64_t slat::hook_entry_t::get_paging_split_state() const
{
    return this->paging_split_state;
}

void slat::hook_entry_t::set_original_pfn(const std::uint64_t original_pfn)
{
    this->lower_original_pfn = static_cast<std::uint32_t>(original_pfn);
    this->higher_original_pfn = original_pfn >> 32;
}

void slat::hook_entry_t::set_shadow_pfn(const std::uint64_t shadow_pfn)
{
    this->lower_shadow_pfn = static_cast<std::uint32_t>(shadow_pfn);
    this->higher_shadow_pfn = shadow_pfn >> 32;
}

void slat::hook_entry_t::set_original_read_access(const std::uint64_t original_read_access_in)
{
    this->original_read_access = original_read_access_in;
}

void slat::hook_entry_t::set_original_write_access(const std::uint64_t original_write_access_in)
{
    this->original_write_access = original_write_access_in;
}

void slat::hook_entry_t::set_original_execute_access(const std::uint64_t original_execute_access_in)
{
    this->original_execute_access = original_execute_access_in;
}

void slat::hook_entry_t::set_paging_split_state(const std::uint64_t paging_split_state_in)
{
    this->paging_split_state = paging_split_state_in;
}

slat::hook_entry_t *slat::hook_entry_t::find(hook_entry_t *list_head, std::uint64_t target_original_4kb_pfn,
                                             hook_entry_t **previous_entry_out)
{
    hook_entry_t *current_entry = list_head;
    hook_entry_t *previous_entry = nullptr;

    while (current_entry != nullptr)
    {
        if (current_entry->get_original_pfn() == target_original_4kb_pfn)
        {
            if (previous_entry_out != nullptr)
            {
                *previous_entry_out = previous_entry;
            }

            return current_entry;
        }

        previous_entry = current_entry;
        current_entry = current_entry->get_next();
    }

    return nullptr;
}

slat::hook_entry_t *slat::hook_entry_t::find_in_2mb_range(hook_entry_t *list_head,
                                                          std::uint64_t target_original_4kb_pfn,
                                                          hook_entry_t *excluding_hook)
{
    hook_entry_t *current_entry = list_head;

    std::uint64_t target_2mb_pfn = target_original_4kb_pfn >> 9;

    while (current_entry != nullptr)
    {
        std::uint64_t current_hook_2mb_pfn = current_entry->get_original_pfn() >> 9;

        if (excluding_hook != current_entry && current_hook_2mb_pfn == target_2mb_pfn)
        {
            return current_entry;
        }

        current_entry = current_entry->get_next();
    }

    return nullptr;
}

slat::hook_entry_t *slat::hook_entry_t::find_closest_in_2mb_range(hook_entry_t *list_head,
                                                                  std::uint64_t target_original_4kb_pfn,
                                                                  hook_entry_t *excluding_hook)
{
    hook_entry_t *current_entry = list_head;

    std::uint64_t target_2mb_pfn = target_original_4kb_pfn >> 9;

    hook_entry_t *closest_entry = nullptr;
    std::int64_t closest_difference = INT64_MAX;

    while (current_entry != nullptr)
    {
        std::uint64_t current_hook_4kb_pfn = current_entry->get_original_pfn();
        std::uint64_t current_hook_2mb_pfn = current_hook_4kb_pfn >> 9;

        if (excluding_hook != current_entry && current_hook_2mb_pfn == target_2mb_pfn)
        {
            std::int64_t current_difference = crt::abs(static_cast<std::int64_t>(current_hook_4kb_pfn) -
                                                       static_cast<std::int64_t>(target_original_4kb_pfn));

            if (current_difference < closest_difference)
            {
                closest_difference = current_difference;
                closest_entry = current_entry;
            }
        }

        current_entry = current_entry->get_next();
    }

    return closest_entry;
}
