#include "winload.h"
#include "../hooks/hooks.h"
#include "../image/image.h"
#include "../bootmgfw/bootmgfw.h"
#include "../structures/ntdef.h"
#include "../hvloader/hvloader.h"

UINT64 pml4_physical_allocation = 0;
UINT64 pdpt_physical_allocation = 0;

hook_data_t winload_load_pe_image_hook_data = { 0 };

int winload_load_pe_image_detour(unknown_param_t a1, unknown_param_t a2, CHAR16* file_path, UINT64* p_image_base, UINT32* p_image_size, unknown_param_t a6, unknown_param_t a7, unknown_param_t a8, unknown_param_t a9, unknown_param_t a10, unknown_param_t a11, unknown_param_t a12, unknown_param_t a13, unknown_param_t a14)
{
    // This internally calls winload.efi!ImgpLoadPEImage. The difference is that this function is exported, 
    // making it a more stable choice since we don't have to locate it via pattern scanning.

    hook_disable(&winload_load_pe_image_hook_data);

    // Actual type is NTSTATUS; we should check that the return value is >=0 before doing anything.
    const int status = ((bl_img_load_pe_image_ex_t)winload_load_pe_image_hook_data.hooked_subroutine_address)(a1, a2, file_path, p_image_base, p_image_size, a6, a7, a8, a9, a10, a11, a12, a13, a14);

    if (status >= 0 && StrStr(file_path, L"hvloader") != NULL)
    {
        hvloader_place_hooks(*p_image_base, *p_image_size);
    }

    hook_enable(&winload_load_pe_image_hook_data);

    return status;
}

EFI_STATUS winload_place_load_pe_image_hook(UINT64 image_base, UINT64 image_size)
{
    // (2026) @xref2000: ImgpLoadPEImage can be called from 2 routines besides BlImgLoadPEImageEx. Across 2 different physical devices and a VM, 
    // I've seen neither of them used to map hvloader.dll. bootmgfw.efi!ImgpLoadPEImage sigs break for me across devices on the exact same winver, 
    // so I would consider this a safer/easier approach even though it technically could miss calls. 
    // 
    // If you'd rather hook ImgpLoadPEImage, you can resolve the call operand at whatever offset it is from this function. The downside to this is 
    // that the offset may differ between winvers. You can use XED (or zydis since it's already included in the usermode dependencies) to locate the 
    // third call and resolve it's operand if you don't want to deal with checking offsets for every single version of winload.efi.

    CHAR8* load_pe_image_subroutine = NULL;

    EFI_STATUS status = image_get_export("BlImgLoadPEImageEx", (UINT8*)image_base, &load_pe_image_subroutine);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    status = hook_create(&winload_load_pe_image_hook_data, load_pe_image_subroutine, (void*)winload_load_pe_image_detour);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    return hook_enable(&winload_load_pe_image_hook_data);
}

EFI_STATUS winload_place_hooks(UINT64 image_base, UINT64 image_size)
{
    return winload_place_load_pe_image_hook(image_base, image_size);
}