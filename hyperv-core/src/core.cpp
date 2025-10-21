#include "..\pch.h"

namespace hyperv
{
static error_code last_error = error_code::success;
static bool library_initialized = false;

bool initialize()
{
    if (library_initialized)
    {
        return true;
    }

    logger::initialize();

    if (!hypercall::initialize())
    {
        last_error = error_code::initialization_failed;
        return false;
    }

    if (!hook::initialize_hook_system())
    {
        last_error = error_code::initialization_failed;
        return false;
    }

    library_initialized = true;
    last_error = error_code::success;
    return true;
}

void shutdown()
{
    if (!library_initialized)
        return;

    hook::cleanup_hook_system();
    logger::shutdown();

    library_initialized = false;
    last_error = error_code::success;
}

error_code get_last_error()
{
    return last_error;
}

void set_last_error(error_code error)
{
    last_error = error;
}
} // namespace hyperv