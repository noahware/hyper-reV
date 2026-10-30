#pragma once
#include <array.hpp>
#include <cstdint>
#include <cstddef>

union parted_addr
{
	struct
	{
		std::uint32_t low;
		std::uint32_t high;
	};

	void* addr;
};

class hook
{
public:
	hook() noexcept = default;

	hook(void* const target, void* const detour)
		:	target_(target)
	{
		constexpr std::size_t low_off = 1;
		constexpr std::size_t high_off = 9;

		cstd::array<std::uint8_t, stub_size> stub_bytes = {
			0x68, 0x21, 0x43, 0x65, 0x87, // push   0xffffffff87654321
			0xC7, 0x44, 0x24, 0x04, 0x78, 0x56, 0x34, 0x12, // mov    DWORD PTR [rsp+0x4],0x12345678
			0xC3 // ret
		};

		const parted_addr addr = {
			.addr = detour
		};

		cstd::crt::memcpy(stub_bytes.data() + low_off, &addr.low, sizeof(std::uint32_t));
		cstd::crt::memcpy(stub_bytes.data() + high_off, &addr.high, sizeof(std::uint32_t));

		cstd::crt::memcpy(orig_bytes_.data(), target, stub_size);
		cstd::crt::memcpy(stub_bytes_.data(), stub_bytes.data(), stub_size);

		enable();
	}

	static hook create(void* const target, void* const detour)
	{
		return hook{ target, detour };
	}

	void enable() noexcept
	{
		cstd::crt::memcpy(target_, stub_bytes_.data(), stub_size);
	}

	void disable() noexcept
	{
		cstd::crt::memcpy(target_, orig_bytes_.data(), stub_size);
	}
	
	template <class T>
	[[nodiscard]] T target() const noexcept
	{
		return reinterpret_cast<T>(target_);
	}


protected:
	constexpr static std::size_t stub_size = 14;

	void* target_ = nullptr;
	cstd::array<std::uint8_t, stub_size> orig_bytes_ = { };
	cstd::array<std::uint8_t, stub_size> stub_bytes_ = { };
};
