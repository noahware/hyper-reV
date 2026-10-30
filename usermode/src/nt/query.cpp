#include "query.hpp"

#include <Windows.h>

extern "C" NTSTATUS NTAPI NtQuerySystemInformation(ULONG info_class, PVOID info_out, ULONG info_size,
                                                   PULONG returned_size);

std::vector<std::uint8_t> nt::query_system_info(const std::uint32_t info_class) noexcept
{
	ULONG size = 0;

	NtQuerySystemInformation(info_class, nullptr, 0, &size);

	if (size == 0)
		return {};

	std::vector<std::uint8_t> buf(size);

	if (NtQuerySystemInformation(info_class, buf.data(), size, &size) < 0)
		return {};

	return buf;
}
