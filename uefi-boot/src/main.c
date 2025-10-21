#include <Library/UefiBootServicesTableLib.h>

#include "bootmgfw/bootmgfw.h"
#include "hyperv_attachment/hyperv_attachment.h"
#include "../arte.h"

const UINT8 _gDriverUnloadImageCount = 1;
const UINT32 _gUefiDriverRevision = 0x200;
CHAR8 *gEfiCallerBaseName = "hyper-reV";

EFI_STATUS
EFIAPI
UefiUnload(IN EFI_HANDLE image_handle)
{
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiMain(IN EFI_HANDLE image_handle, IN EFI_SYSTEM_TABLE *system_table)
{
    EFI_HANDLE device_handle = NULL;
    EFI_STATUS status = bootmgfw_restore_original_file(&device_handle);

    if (status != EFI_SUCCESS)
    {
        return status;
    }

    status = hyperv_attachment_set_up();
    if (status != EFI_SUCCESS)
    {
        return status;
    }

    return bootmgfw_run_original_image(image_handle, device_handle);
}
