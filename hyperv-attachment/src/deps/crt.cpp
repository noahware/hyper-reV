#pragma function(memset, memcpy)

#include "../mem/alloc/slab.hpp"

#include <cstdint>
#include <crt.hpp>

#include <intrin.h>

void* cstd::crt::malloc(const std::size_t size)
{
	return mem::slab::alloc(size);
}

void cstd::crt::free(void* const buffer)
{
	mem::slab::free(buffer);
}

void cstd::crt::memcpy(void* const destination, const void* const source, const std::size_t size)
{
	__movsb(static_cast<std::uint8_t*>(destination), static_cast<const std::uint8_t*>(source), size);
}

void cstd::crt::memset(void* const destination, const std::int32_t value, const std::size_t size)
{
	__stosb(static_cast<std::uint8_t*>(destination), static_cast<std::uint8_t>(value), size);
}

void* operator new(const std::size_t size)
{
	return cstd::crt::malloc(size);
}

void* operator new[](const std::size_t size)
{
	return cstd::crt::malloc(size);
}

void* operator new([[maybe_unused]] const std::size_t size, void* const buffer) noexcept
{
	return buffer;
}

void* operator new[]([[maybe_unused]] const std::size_t size, void* const buffer) noexcept
{
	return buffer;
}

void operator delete(void* const buffer) noexcept
{
	cstd::crt::free(buffer);
}

void operator delete[](void* const buffer) noexcept
{
	cstd::crt::free(buffer);
}

void operator delete(void* const buffer, [[maybe_unused]] const std::size_t size) noexcept
{
	cstd::crt::free(buffer);
}

void operator delete[](void* const buffer, [[maybe_unused]] const std::size_t size) noexcept
{
	cstd::crt::free(buffer);
}

// The attachment never unloads, so static destructors would never run anyway.
// Provide a no-op stub so any registration the compiler emits links cleanly.
extern "C" int atexit(void (*)())
{
	return 0;
}
