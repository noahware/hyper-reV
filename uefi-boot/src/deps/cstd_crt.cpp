extern "C" {
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
}

#include <intrin.h>
#include <crt.hpp>

#include "../arch/cpu.hpp"

namespace cstd::crt
{
	void* malloc(size_t size)
	{
		return AllocatePool(static_cast<UINTN>(size));
	}

	void free(void* buffer)
	{
		if (buffer != nullptr)
		{
			FreePool(buffer);
		}
	}

	void memcpy(void* destination, const void* source, size_t size)
	{
		const arch::wp_guard guard;
		__movsb(static_cast<unsigned char*>(destination), static_cast<const unsigned char*>(source), size);
	}

	void memset(void* destination, int32_t value, size_t size)
	{
		const arch::wp_guard guard;
		__stosb(static_cast<unsigned char*>(destination), static_cast<unsigned char>(value), size);
	}
}

extern "C" int atexit(void (*)())
{
	return 0;
}

void* operator new(cstd::size_t size)
{
	void* buffer = cstd::crt::malloc(size);
	ASSERT(buffer != nullptr);
	return buffer;
}

void* operator new[](cstd::size_t size)
{
	return ::operator new(size);
}

void* operator new(cstd::size_t, void* buffer) noexcept
{
	return buffer;
}

void* operator new[](cstd::size_t, void* buffer) noexcept
{
	return buffer;
}

void operator delete(void* buffer) noexcept
{
	cstd::crt::free(buffer);
}

void operator delete[](void* buffer) noexcept
{
	::operator delete(buffer);
}

void operator delete(void* buffer, cstd::size_t) noexcept
{
	cstd::crt::free(buffer);
}

void operator delete[](void* buffer, cstd::size_t) noexcept
{
	::operator delete(buffer);
}
