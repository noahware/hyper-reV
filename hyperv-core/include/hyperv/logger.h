#pragma once
#include <cstdint>
#include <cstdio>
#include <windows.h>

namespace hyperv::logger
{
enum class log_level
{
    trace,
    debug,
    info,
    warning,
    error,
    critical
};

void initialize();
void shutdown();

void log(log_level level, const char *format, ...);
void log_trace(const char *format, ...);
void log_debug(const char *format, ...);
void log_info(const char *format, ...);
void log_warning(const char *format, ...);
void log_error(const char *format, ...);
void log_critical(const char *format, ...);
void log(const char *format, ...);
} // namespace hyperv::logger