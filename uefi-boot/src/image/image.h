#pragma once
#include <Library/UefiLib.h>
#include <IndustryStandard/PeImage.h>
#include <Protocol/LoadedImage.h>

EFI_STATUS load_image(EFI_HANDLE* loaded_image_handle_out, BOOLEAN boot_policy, EFI_HANDLE parent_image_handle, EFI_DEVICE_PATH* device_path);
EFI_STATUS unload_image(EFI_HANDLE image_handle);
EFI_STATUS start_image(EFI_HANDLE image_handle);
EFI_STATUS get_image_info(EFI_LOADED_IMAGE** image_info_out, EFI_HANDLE image_handle);
EFI_STATUS scan_image(CHAR8** location_out, CHAR8* scan_base, UINT64 scan_max_size, UINT8* pattern, UINT8* mask);
EFI_IMAGE_NT_HEADERS64* image_get_nt_headers(UINT8* image_base);
EFI_STATUS image_get_export(const CHAR8* name, UINT8* image_base, CHAR8** export_out);