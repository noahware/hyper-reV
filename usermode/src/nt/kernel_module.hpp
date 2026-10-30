#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

namespace nt
{
	struct kernel_module_info
	{
		std::uintptr_t base = 0;
		std::uint32_t size = 0;
		std::string_view name;
		std::string_view path;

		explicit operator bool() const noexcept
		{
			return base != 0;
		}
	};

	struct raw_kernel_module;

	// Snapshot of the loaded kernel modules at construction time.
	// Owns the backing buffer; kernel_module_info views are valid for its lifetime.
	class kernel_modules
	{
	public:
		kernel_modules() noexcept;

		explicit operator bool() const noexcept
		{
			return !buf_.empty();
		}

		[[nodiscard]] std::size_t count() const noexcept;
		[[nodiscard]] kernel_module_info operator[](std::size_t i) const noexcept;
		[[nodiscard]] kernel_module_info find(std::string_view name) const noexcept;

		struct iterator
		{
			const raw_kernel_module* ptr;

			kernel_module_info operator*() const noexcept;
			iterator& operator++() noexcept;

			bool operator!=(const iterator& o) const noexcept
			{
				return ptr != o.ptr;
			}
		};

		[[nodiscard]] iterator begin() const noexcept;
		[[nodiscard]] iterator end() const noexcept;

	private:
		std::vector<std::uint8_t> buf_;
	};
}
