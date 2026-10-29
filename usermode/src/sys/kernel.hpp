#pragma once
#include "../hypercall/hypercall.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>

namespace sys::kern
{
	struct kernel_module
	{
		std::uintptr_t base = 0;
		std::uint32_t size = 0;
		std::unordered_map<std::string, std::uintptr_t> exports;
	};

	inline std::uint64_t curr_cr3 = 0;
	inline std::unordered_map<std::string, kernel_module> modules_list = {};

	bool init() noexcept;

	void parse_modules() noexcept;

	template <class T>
	T read(const std::uintptr_t va, const std::uint64_t cr3 = curr_cr3) noexcept
	{
		T out{};
		hc::virt_copy(cr3, va, curr_cr3, reinterpret_cast<std::uintptr_t>(&out), sizeof(T));
		return out;
	}

	template <class T>
	bool read(const std::uintptr_t va, const std::span<T> out, const std::uint64_t cr3 = curr_cr3) noexcept
	{
		return hc::virt_copy(cr3, va, curr_cr3, reinterpret_cast<std::uintptr_t>(out.data()), out.size_bytes());
	}

	template <class T>
	bool write(const std::uintptr_t va, const T& in, const std::uint64_t cr3 = curr_cr3) noexcept
	{
		return hc::virt_copy(curr_cr3, reinterpret_cast<std::uintptr_t>(&in), cr3, va, sizeof(T));
	}

	template <class T>
	bool write(const std::uintptr_t va, const std::span<const T> in, const std::uint64_t cr3 = curr_cr3) noexcept
	{
		return hc::virt_copy(curr_cr3, reinterpret_cast<std::uintptr_t>(in.data()), cr3, va, in.size_bytes());
	}
}
