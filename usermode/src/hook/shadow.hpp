#pragma once
#include "../arch/arch.hpp"
#include "../hypercall/hypercall.hpp"
#include "../nt/alloc.hpp"
#include "../sys/kernel.hpp"
#include "../util/log.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <utility>

namespace hook
{
	class shadow
	{
	public:
		shadow() noexcept = default;

		~shadow() noexcept
		{
			reset();
		}

		shadow(shadow&& other) noexcept
			: mem_(std::move(other.mem_)),
			  gpa_(std::exchange(other.gpa_, 0))
		{
		}

		shadow& operator=(shadow&& other) noexcept
		{
			if (this != &other)
			{
				reset();
				mem_ = std::move(other.mem_);
				gpa_ = std::exchange(other.gpa_, 0);
			}

			return *this;
		}

		shadow(const shadow&) = delete;
		shadow& operator=(const shadow&) = delete;

		static std::optional<shadow> create(const std::uintptr_t page_va,
		                                    const std::size_t off,
		                                    const std::span<const std::uint8_t> patch) noexcept
		{
			nt::alloc mem(arch::page_size);
			if (!mem)
			{
				LOG_ERR("hook: shadow alloc failed");
				return std::nullopt;
			}

			mem.lock();

			if (!sys::kern::read(page_va, mem.bytes()))
			{
				LOG_ERR("hook: snapshot failed for {:#x}", page_va);
				return std::nullopt;
			}

			if (!patch.empty())
				std::memcpy(mem.get() + off, patch.data(), patch.size());

			const auto target_gpa = hc::translate_va(sys::kern::curr_cr3, page_va);
			const auto shadow_gpa = hc::translate_va(sys::kern::curr_cr3,
			                                         reinterpret_cast<std::uintptr_t>(mem.get()));

			if (!target_gpa || !shadow_gpa || !hc::hook_add(target_gpa, shadow_gpa))
			{
				LOG_ERR("hook: hook_add failed for {:#x}", page_va);
				return std::nullopt;
			}

			return shadow{ std::move(mem), target_gpa };
		}

		explicit operator bool() const noexcept
		{
			return gpa_ != 0;
		}

		// leaves the hook and its page in place
		void release() noexcept
		{
			gpa_ = 0;
			(void)mem_.release();
		}

		void reset() noexcept
		{
			if (gpa_)
				hc::hook_remove(std::exchange(gpa_, 0));

			mem_.reset();
		}

		[[nodiscard]] std::uint8_t* data() const noexcept { return mem_.get(); }

	private:
		shadow(nt::alloc mem, const std::uintptr_t gpa) noexcept
			: mem_(std::move(mem)),
			  gpa_(gpa)
		{
		}

		nt::alloc mem_;
		std::uintptr_t gpa_ = 0;
	};
}
