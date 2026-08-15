#pragma once
#include <Library/UefiLib.h>
#include <ia32-doc/ia32_compact.h>

typedef UINT64 unknown_param_t;
typedef int(*bl_img_load_pe_image_ex_t)(unknown_param_t a1, unknown_param_t a2, CHAR16* file_path, UINT64* p_image_base, UINT32* p_image_size, unknown_param_t a6, unknown_param_t a7, unknown_param_t a8, unknown_param_t a9, unknown_param_t a10, unknown_param_t a11, unknown_param_t a12, unknown_param_t a13, unknown_param_t a14);

extern UINT64 pml4_physical_allocation;
extern UINT64 pdpt_physical_allocation;

EFI_STATUS winload_place_hooks(UINT64 image_base, UINT64 image_size);
