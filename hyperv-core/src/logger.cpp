#include "..\pch.h"
#include <cstdarg>
#include <ctime>
#include <mutex>

//#define DISABLE_LOGGING 1

namespace hyperv::logger
{
namespace
{
std::mutex log_mutex;
bool initialized = false;
log_level min_log_level = log_level::trace;

const char *level_to_string(log_level level)
{
    switch (level)
    {
    case log_level::trace:
        return "TRACE";
    case log_level::debug:
        return "DEBUG";
    case log_level::info:
        return "INFO";
    case log_level::warning:
        return "WARN";
    case log_level::error:
        return "ERROR";
    case log_level::critical:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

void get_timestamp(char *buffer, size_t buffer_size)
{
    time_t now = time(nullptr);
    struct tm time_info;
    localtime_s(&time_info, &now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &time_info);
}

void log_with_level(log_level level, const char *format, va_list args)
{
#if defined(DISABLE_LOGGING)
    return;
#else
    if (level < min_log_level)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(log_mutex);

    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    char message[1024];
    vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);

    printf("[%s] [%s] %s\n", timestamp, level_to_string(level), message);
#endif
}
} // namespace

void initialize()
{
    std::lock_guard<std::mutex> lock(log_mutex);
    if (!initialized)
    {
        initialized = true;
    }
}

void shutdown()
{
    std::lock_guard<std::mutex> lock(log_mutex);
    if (initialized)
    {
        initialized = false;
    }
}

void log(log_level level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_with_level(level, format, args);
    va_end(args);
}

void log_trace(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_with_level(log_level::trace, format, args);
    va_end(args);
}

void log_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_with_level(log_level::debug, format, args);
    va_end(args);
}

void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_with_level(log_level::info, format, args);
    va_end(args);
}

void log_warning(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_with_level(log_level::warning, format, args);
    va_end(args);
}

void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_with_level(log_level::error, format, args);
    va_end(args);
}

void log_critical(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_with_level(log_level::critical, format, args);
    va_end(args);
}

// Legacy function for backward compatibility
void log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_with_level(log_level::info, format, args);
    va_end(args);
}
} // namespace hyperv::logger