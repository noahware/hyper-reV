#include "bootmgfw.h"

#include "../winload/winload.h"
#include "../memory_manager/memory_manager.h"
#include "../hooks/hooks.h"
#include "../image/image.h"
#include "../disk/disk.h"
#include "../structures/ntdef.h"
#include <Library/UefiBootServicesTableLib.h>

UINT64 uefi_boot_physical_base_address = 0;
UINT32 uefi_boot_image_size = 0;

#define d_bootmgfw_path L"\\efi\\microsoft\\boot\\bootmgfw.efi"
#define d_path_original_bootmgfw L"\\efi\\microsoft\\boot\\bootmgfw.original.efi"

hook_data_t bootmgfw_load_pe_image_hook_data = {0};

EFI_STATUS write_original_bootmgfw_back(EFI_FILE_INFO *original_bootmgfw_file_info, EFI_FILE_PROTOCOL *bootmgfw_file,
                                        void *buffer, UINT64 buffer_size)
{
    EFI_STATUS status = disk_write_file(bootmgfw_file, buffer, buffer_size);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    EFI_FILE_INFO *bootmgfw_file_info = NULL;
    UINT64 file_info_size = 0;

    status = disk_get_generic_file_info(&bootmgfw_file_info, &file_info_size, bootmgfw_file);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    bootmgfw_file_info->CreateTime = original_bootmgfw_file_info->CreateTime;
    bootmgfw_file_info->ModificationTime = original_bootmgfw_file_info->ModificationTime;

    status = disk_set_generic_file_info(bootmgfw_file, bootmgfw_file_info, file_info_size);

    mm_free_pool(bootmgfw_file_info);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS bootmgfw_restore_original_file(EFI_HANDLE *device_handle_out)
{
    EFI_FILE_PROTOCOL *bootmgfw_original_file = NULL;

    EFI_STATUS status = disk_open_file(&bootmgfw_original_file, device_handle_out, d_path_original_bootmgfw,
                                       EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, EFI_FILE_SYSTEM);

    if (status == EFI_SUCCESS)
    {
        EFI_FILE_INFO *original_bootmgfw_file_info = NULL;
        UINT64 file_info_size = 0;

        status = disk_get_generic_file_info(&original_bootmgfw_file_info, &file_info_size, bootmgfw_original_file);

        if (status == EFI_SUCCESS)
        {
            UINT64 original_bootmgfw_buffer_size = original_bootmgfw_file_info->FileSize;
            void *original_bootmgfw_buffer = NULL;

            status = disk_load_file(bootmgfw_original_file, &original_bootmgfw_buffer, original_bootmgfw_buffer_size);

            if (status == EFI_SUCCESS)
            {
                EFI_FILE_PROTOCOL *bootmgfw_file = NULL;

                status = disk_open_file(&bootmgfw_file, device_handle_out, d_bootmgfw_path,
                                        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, EFI_FILE_SYSTEM);

                if (status == EFI_SUCCESS)
                {
                    status = write_original_bootmgfw_back(original_bootmgfw_file_info, bootmgfw_file,
                                                          original_bootmgfw_buffer, original_bootmgfw_buffer_size);

                    disk_close_file(bootmgfw_file);
                }

                mm_free_pool(original_bootmgfw_buffer);
            }

            mm_free_pool(original_bootmgfw_file_info);
        }

        disk_delete_file(bootmgfw_original_file);
    }

    return status;
}

UINT64 bootmgfw_load_pe_image_detour(bl_file_info_t *file_info, INT32 a2, UINT64 *image_base, UINT32 *image_size,
                                     UINT64 *a5, UINT32 *a6, UINT32 *a7, UINT64 a8, UINT64 a9, unknown_param_t a10,
                                     unknown_param_t a11, unknown_param_t a12, unknown_param_t a13, unknown_param_t a14,
                                     unknown_param_t a15)
{
    hook_disable(&bootmgfw_load_pe_image_hook_data);

    boot_load_pe_image_t original_subroutine =
        (boot_load_pe_image_t)bootmgfw_load_pe_image_hook_data.hooked_subroutine_address;

    UINT64 return_value =
        original_subroutine(file_info, a2, image_base, image_size, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);

    if (StrStr(file_info->file_name, L"winload.efi") != NULL)
    {
        if (winload_place_hooks(*image_base, (UINT64)*image_size) == EFI_SUCCESS)
        {

            Print(L"success in winload hooks");
        }
        else
        {
            Print(L"error in winload hooks\n");
        }

        return return_value;
    }

    hook_enable(&bootmgfw_load_pe_image_hook_data);

    return return_value;
}

EFI_STATUS bootmgfw_place_load_pe_image_hook(EFI_LOADED_IMAGE *bootmgfw_image_info)
{
    CHAR8 *code_ref_to_load_pe_image = NULL;

    // ImgpLoadPEImage
    EFI_STATUS status =
        scan_image(&code_ref_to_load_pe_image, bootmgfw_image_info->ImageBase, bootmgfw_image_info->ImageSize,
                   d_boot_load_pe_image_pattern, d_boot_load_pe_image_mask);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    CHAR8 *load_pe_image_subroutine = (code_ref_to_load_pe_image + 10) + *(UINT32 *)(code_ref_to_load_pe_image + 6);

    status =
        hook_create(&bootmgfw_load_pe_image_hook_data, load_pe_image_subroutine, (void *)bootmgfw_load_pe_image_detour);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    return hook_enable(&bootmgfw_load_pe_image_hook_data);
}

EFI_STATUS bootmgfw_place_hooks(EFI_HANDLE bootmgfw_image_handle)
{
    EFI_LOADED_IMAGE *bootmgfw_image_info = NULL;

    EFI_STATUS status = get_image_info(&bootmgfw_image_info, bootmgfw_image_handle);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    return bootmgfw_place_load_pe_image_hook(bootmgfw_image_info);
}

EFI_STATUS parse_uefi_boot_image_info(EFI_HANDLE image_handle)
{
    EFI_LOADED_IMAGE *image_info = NULL;

    EFI_STATUS status = get_image_info(&image_info, image_handle);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    uefi_boot_physical_base_address = (UINT64)image_info->ImageBase;
    uefi_boot_image_size = (UINT32)image_info->ImageSize;

    return EFI_SUCCESS;
}

EFI_STATUS bootmgfw_run_original_image(EFI_HANDLE parent_image_handle, EFI_HANDLE device_handle)
{
    EFI_STATUS status = parse_uefi_boot_image_info(parent_image_handle);

    if (status == EFI_SUCCESS)
    {
        EFI_DEVICE_PATH *device_path = NULL;

        status = disk_get_device_path(&device_path, device_handle, d_bootmgfw_path);

        if (status == EFI_SUCCESS)
        {
            EFI_HANDLE loaded_image = NULL;

            status = load_image(&loaded_image, TRUE, parent_image_handle, device_path);

            if (status == EFI_SUCCESS)
            {
                status = bootmgfw_place_hooks(loaded_image);

                if (status == EFI_SUCCESS)
                {
                    status = start_image(loaded_image);
                }
                else
                {
                    unload_image(loaded_image);
                }
            }
            else
            {
                unload_image(loaded_image);
            }
        }
    }

    return status;
}