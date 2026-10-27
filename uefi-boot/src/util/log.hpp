#pragma once
extern "C" {
#include <Library/UefiLib.h>
}

#define LOG(fmt, ...) Print((CHAR16*)fmt L"\r\n" __VA_OPT__(,) __VA_ARGS__)
