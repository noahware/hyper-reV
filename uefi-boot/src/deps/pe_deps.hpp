#pragma once
#include <cstddef>
#include <cstdint>

#include <array.hpp>
#include <span.hpp>
#include <vector.hpp>
#include <string_view.hpp>
#include <charconv.hpp>
#include <ranges.hpp>
#include <views.hpp>
#include <crt.hpp>

namespace pe
{
	template <class T>
	using span_t = cstd::span<T>;

	template <class T, std::size_t N>
	using array_t = cstd::array<T, N>;

	template <class T>
	using vector_t = cstd::vector<T>;

	using string_view_t = cstd::string_view;

	using cstd::crt::memcpy;

	using cstd::from_chars;

	namespace ranges
	{
		using cstd::ranges::find;
	}

	namespace views
	{
		using cstd::views::filter;
		using cstd::views::iota;
		using cstd::views::join;
		using cstd::views::take_while;
		using cstd::views::transform;
	}
}
