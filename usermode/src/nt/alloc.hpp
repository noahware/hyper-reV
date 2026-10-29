#pragma once
#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace nt
{
	// owns a VirtualAlloc reservation and frees it on scope exit
	class alloc
	{
	public:
		alloc() noexcept = default;

		explicit alloc(const std::size_t size, const std::uint32_t protect = PAGE_READWRITE) noexcept
			: mem_(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, protect)),
			  size_(size)
		{
			if (!mem_)
				size_ = 0;
		}

		~alloc() noexcept
		{
			reset();
		}

		alloc(alloc&& other) noexcept
			: mem_(std::exchange(other.mem_, nullptr)),
			  size_(std::exchange(other.size_, 0))
		{
		}

		alloc& operator=(alloc&& other) noexcept
		{
			if (this != &other)
			{
				reset();
				mem_ = std::exchange(other.mem_, nullptr);
				size_ = std::exchange(other.size_, 0);
			}

			return *this;
		}

		alloc(const alloc&) = delete;
		alloc& operator=(const alloc&) = delete;

		explicit operator bool() const noexcept
		{
			return mem_ != nullptr;
		}

		// pins the pages resident so their physical address stays put
		bool lock() const noexcept
		{
			return mem_ && VirtualLock(mem_, size_);
		}

		// gives up ownership, so the pages outlive this object
		[[nodiscard]] void* release() noexcept
		{
			size_ = 0;
			return std::exchange(mem_, nullptr);
		}

		void reset() noexcept
		{
			if (mem_)
				VirtualFree(std::exchange(mem_, nullptr), 0, MEM_RELEASE);

			size_ = 0;
		}

		template <class T = std::uint8_t>
		[[nodiscard]] T* get() const noexcept { return static_cast<T*>(mem_); }

		[[nodiscard]] std::size_t size() const noexcept { return size_; }
		[[nodiscard]] std::span<std::uint8_t> bytes() const noexcept { return { get(), size_ }; }

	private:
		void* mem_ = nullptr;
		std::size_t size_ = 0;
	};
}
