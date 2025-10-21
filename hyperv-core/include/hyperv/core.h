#pragma once
#include "hook.h"
#include "hypercall.h"
#include "memory.h"

namespace hyperv
{
bool initialize();
void shutdown();

// Error handling
enum class error_code : uint8_t
{
    success = 0,
    initialization_failed,
    invalid_parameter,
    memory_operation_failed,
    hook_operation_failed,
    hypercall_failed
};

// Get last error
error_code get_last_error();
void set_last_error(error_code error);
} // namespace hyperv