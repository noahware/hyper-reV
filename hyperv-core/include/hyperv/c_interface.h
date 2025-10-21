
#pragma once
#include <stdint.h>
#ifdef _MSC_VER
#define HYPERV_API __declspec(dllexport)
#else
#define HYPERV_API
#endif
#ifdef __cplusplus
extern "C"
{
#endif
    HYPERV_API bool hyperv_initialize();
    HYPERV_API void hyperv_shutdown();

    HYPERV_API uint64_t hyperv_memory_read_physical(void *destination_physical_address, uint64_t guest_physical_address,
                                                    uint64_t size);
    HYPERV_API uint64_t hyperv_memory_write_physical(const void *source_physical_address,
                                                     uint64_t guest_physical_address, uint64_t size);

    HYPERV_API uint64_t hyperv_read_host_physical_memory(void *destination_buffer, uint64_t host_physical_address,
                                                         uint64_t size);
    HYPERV_API uint64_t hyperv_write_host_physical_memory(const void *source_buffer, uint64_t host_physical_address,
                                                          uint64_t size);

    HYPERV_API uint64_t hyperv_translate_virtual_to_physical(uint64_t virtual_address);
    HYPERV_API uint64_t hyperv_memory_get_current_cr3();

    HYPERV_API int hyperv_get_last_error();
#ifdef __cplusplus
}
#endif