#pragma once
#include <cstdio>
#include <format>

#define LOG_INFO(...) std::printf("%s\n", std::format(__VA_ARGS__).c_str())
#define LOG_WARN(...) std::printf("%s\n", std::format(__VA_ARGS__).c_str())
#define LOG_ERR(...) std::printf("%s\n", std::format(__VA_ARGS__).c_str())
