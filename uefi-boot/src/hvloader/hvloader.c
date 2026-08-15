#include "hvloader.h"

#include "../bootmgfw/bootmgfw.h"
#include "../hooks/hooks.h"
#include "../image/image.h"
#include "../structures/virtual_address.h"
#include "../memory_manager/memory_manager.h"
#include "../hyperv_attachment/hyperv_attachment.h"
#include "../winload/winload.h"

hook_data_t hvloader_launch_hv_hook_data = { 0 };
hook_data_t hv_vmexit_hook_data = { 0 };

typedef void(*hvloader_launch_hv_t)(cr3 a1, virtual_address_t a2, UINT64 a3, UINT64 a4);

void set_up_identity_map(pml4e_64* pml4e)
{
    pdpte_1gb_64* pdpt = (pdpte_1gb_64*)pdpt_physical_allocation;

    pml4e->flags = 0;
    pml4e->page_frame_number = pdpt_physical_allocation >> 12;
    pml4e->present = 1;
    pml4e->write = 1;

    for (UINT64 i = 0; i < 512; i++)
    {
        pdpte_1gb_64* pdpte = &pdpt[i];

        pdpte->flags = 0;
        pdpte->page_frame_number = i;
        pdpte->present = 1;
        pdpte->write = 1;
        pdpte->large_page = 1;
    }
}

void load_identity_map_into_hyperv_cr3(cr3 identity_map_cr3, cr3 hyperv_cr3, pml4e_64 identity_map_pml4e, pml4e_64* initial_hyperv_pml4e)
{
    AsmWriteCr3(identity_map_cr3.flags);

    pml4e_64* hyperv_pml4 = (pml4e_64*)(hyperv_cr3.address_of_page_directory << 12);

    *initial_hyperv_pml4e = hyperv_pml4[0];

    hyperv_pml4[0] = identity_map_pml4e;
    hyperv_pml4[255] = identity_map_pml4e;
}

void restore_initial_hyperv_pml4e(cr3 identity_map_cr3, cr3 hyperv_cr3, pml4e_64 initial_hyperv_pml4e)
{
    AsmWriteCr3(identity_map_cr3.flags);

    pml4e_64* hyperv_pml4 = (pml4e_64*)(hyperv_cr3.address_of_page_directory << 12);

    hyperv_pml4[0] = initial_hyperv_pml4e;
}

// must have identity map in 0th pml4e
UINT8 is_page_executable(cr3 cr3_to_search, virtual_address_t page)
{
    pml4e_64* pml4 = (pml4e_64*)(cr3_to_search.address_of_page_directory << 12);
    pml4e_64 pml4e = pml4[page.pml4_idx];

    if (pml4e.present == 0 || pml4e.execute_disable == 1)
    {
        return 0;
    }

    pdpte_64* pdpt = (pdpte_64*)(pml4e.page_frame_number << 12);
    pdpte_64 pdpte = pdpt[page.pdpt_idx];

    if (pdpte.present == 0 || pdpte.execute_disable == 1)
    {
        return 0;
    }

    if (pdpte.large_page == 1)
    {
        return 1;
    }

    pde_64* pd = (pde_64*)(pdpte.page_frame_number << 12);
    pde_64 pde = pd[page.pd_idx];

    if (pde.present == 0 || pde.execute_disable == 1)
    {
        return 0;
    }

    if (pde.large_page == 1)
    {
        return 1;
    }

    pte_64* pt = (pte_64*)(pde.page_frame_number << 12);
    pte_64 pte = pt[page.pt_idx];

    if (pte.present == 0 || pte.execute_disable == 1)
    {
        return 0;
    }

    return 1;
}

UINT64 find_hyperv_text_base(cr3 hyperv_cr3, virtual_address_t entry_point)
{
    virtual_address_t text_address = entry_point;

    while (is_page_executable(hyperv_cr3, text_address) == 1)
    {
        text_address.address -= 0x1000;
    }

    return text_address.address + 0x1000;
}

UINT64 find_hyperv_text_end(cr3 hyperv_cr3, virtual_address_t entry_point)
{
    virtual_address_t text_address = entry_point;

    while (is_page_executable(hyperv_cr3, text_address) == 1)
    {
        text_address.address += 0x1000;
    }

    return text_address.address - 0x1000;
}

void set_up_hyperv_hooks(cr3 hyperv_cr3, virtual_address_t entry_point)
{
    AsmWriteCr3(hyperv_cr3.flags);

    UINT64 hyperv_text_base = find_hyperv_text_base(hyperv_cr3, entry_point);
    UINT64 hyperv_text_end = find_hyperv_text_end(hyperv_cr3, entry_point);
    UINT64 hyperv_text_size = hyperv_text_end - hyperv_text_base;

    if (hyperv_text_base != 0)
    {
        UINT8* hyperv_attachment_entry_point = NULL;

        EFI_STATUS status = hyperv_attachment_get_relocated_entry_point(&hyperv_attachment_entry_point);

        if (status == EFI_SUCCESS)
        {
            CHAR8* code_ref_to_vmexit_handler = NULL;

            UINT8 is_intel = 0;

            // search for AMD's vmexit handler
            status = scan_image(&code_ref_to_vmexit_handler, (CHAR8*)hyperv_text_base, hyperv_text_size, "\xE8\x00\x00\x00\x00\x48\x89\x04\x24\xE9", "x????xxxxx");

            if (status == EFI_NOT_FOUND)
            {
                // search for Intel's vmexit handler
                status = scan_image(&code_ref_to_vmexit_handler, (CHAR8*)hyperv_text_base, hyperv_text_size, "\xE8\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x74\x0D", "x????x????xx");

                is_intel = 1;
            }

            if (status == EFI_SUCCESS)
            {
                INT32 original_vmexit_handler_rva = *(INT32*)(code_ref_to_vmexit_handler + 1);
                CHAR8* original_vmexit_handler = (code_ref_to_vmexit_handler + 5) + original_vmexit_handler_rva;

                UINT8* hyperv_attachment_vmexit_handler_detour = NULL;

                CHAR8* get_vmcb_gadget = NULL;

                if (is_intel == 0)
                {
                    status = scan_image(&get_vmcb_gadget, (CHAR8*)hyperv_text_base, hyperv_text_size, "\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x48\x8B\x88\x00\x00\x00\x00\x48\x8B\x81\x00\x00\x00\x00\x48\x8B", "xxxxx????xxx????xxx????xx");

                    if (status != EFI_SUCCESS)
                    {
                        return;
                    }
                }

                UINT64 heap_physical_base = hyperv_attachment_heap_allocation_base;
                UINT64 heap_physical_usable_base = hyperv_attachment_heap_allocation_usable_base;
                UINT64 heap_total_size = hyperv_attachment_heap_allocation_size;

                hyperv_attachment_invoke_entry_point(&hyperv_attachment_vmexit_handler_detour, hyperv_attachment_entry_point, original_vmexit_handler, heap_physical_base, heap_physical_usable_base, heap_total_size, uefi_boot_physical_base_address, uefi_boot_image_size, get_vmcb_gadget);

                CHAR8* code_cave = NULL;

                status = scan_image(&code_cave, (CHAR8*)hyperv_text_base, hyperv_text_size, "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC", "xxxxxxxxxxxxxxxx");

                if (status == EFI_SUCCESS)
                {
                    status = hook_create(&hv_vmexit_hook_data, code_cave, hyperv_attachment_vmexit_handler_detour);

                    if (status == EFI_SUCCESS)
                    {
                        hook_enable(&hv_vmexit_hook_data);

                        UINT32 new_call_rva = (UINT32)(code_cave - (code_ref_to_vmexit_handler + 5));

                        mm_copy_memory(code_ref_to_vmexit_handler + 1, (UINT8*)&new_call_rva, sizeof(new_call_rva));
                    }
                }
            }
        }
    }
}

void hvloader_launch_hv_detour(cr3 hyperv_cr3, virtual_address_t hyperv_entry_point, UINT64 jmp_gadget, UINT64 kernel_cr3)
{
    hook_disable(&hvloader_launch_hv_hook_data);

    pml4e_64* virtual_pml4 = (pml4e_64*)pml4_physical_allocation;

    set_up_identity_map(&virtual_pml4[0]);

    UINT64 original_cr3 = AsmReadCr3();

    cr3 identity_map_cr3 = { .address_of_page_directory = pml4_physical_allocation >> 12 };

    pml4e_64 initial_hyperv_pml4e = { 0 };

    load_identity_map_into_hyperv_cr3(identity_map_cr3, hyperv_cr3, virtual_pml4[0], &initial_hyperv_pml4e);

    set_up_hyperv_hooks(hyperv_cr3, hyperv_entry_point);

    restore_initial_hyperv_pml4e(identity_map_cr3, hyperv_cr3, initial_hyperv_pml4e);

    AsmWriteCr3(original_cr3);

    hvloader_launch_hv_t original_subroutine = (hvloader_launch_hv_t)hvloader_launch_hv_hook_data.hooked_subroutine_address;

    original_subroutine(hyperv_cr3, hyperv_entry_point, jmp_gadget, kernel_cr3);
}

EFI_STATUS hvloader_place_hooks(UINT64 image_base, UINT64 image_size)
{
    CHAR8* hvloader_launch_hv = NULL;

    EFI_STATUS status = scan_image(&hvloader_launch_hv, (CHAR8*)image_base, image_size, "\x48\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x89\x25", "xxxxxxxxxxxxxxxx?xxx");

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    status = hook_create(&hvloader_launch_hv_hook_data, hvloader_launch_hv, (void*)hvloader_launch_hv_detour);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    return hook_enable(&hvloader_launch_hv_hook_data);
}
