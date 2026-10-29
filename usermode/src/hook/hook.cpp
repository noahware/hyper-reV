#include "hook.hpp"
#include "arena.hpp"
#include "encode.hpp"
#include "orig_bytes.hpp"
#include "shadow.hpp"

#include "../arch/arch.hpp"
#include "../util/log.hpp"

#include <algorithm>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	using namespace hook;

	struct hook_record
	{
		shadow target;
		shadow overflow;
		std::uintptr_t stub_va = 0;
		std::size_t stub_size = 0;
	};

	std::unordered_map<std::uintptr_t, hook_record> list;

	// pre, the optional log hypercall, the displaced original, post, then back
	std::vector<std::uint8_t> collect_stub_bytes(const std::span<const std::uint8_t> pre,
	                                             const std::span<const std::uint8_t> orig_bytes,
	                                             const std::span<const std::uint8_t> post,
	                                             const std::uintptr_t return_va,
	                                             const bool monitor) noexcept
	{
		std::vector<std::uint8_t> out;

		const auto append = [&out](const std::span<const std::uint8_t> bytes) noexcept
		{
			out.insert(out.end(), bytes.begin(), bytes.end());
		};

		append(pre);

		if (monitor)
			append(encode_monitor_stub());

		append(orig_bytes);
		append(post);
		append(encode_abs_jmp(return_va));

		return out;
	}

	// the entry jmp can cross a page boundary, so the tail needs a second shadow
	bool handle_page_overflow(hook_record& rec,
	                          const std::uintptr_t page,
	                          const std::span<const std::uint8_t> tail) noexcept
	{
		auto overflow = shadow::create(page + arch::page_size, 0, tail);
		if (!overflow)
			return false;

		rec.overflow = std::move(*overflow);
		return true;
	}

	std::optional<hook_record> redirect_entry(const std::uintptr_t target_va,
	                                          const std::uintptr_t stub_va) noexcept
	{
		const auto entry = encode_abs_jmp(stub_va);
		const auto page = arch::page_base(target_va);
		const auto off = target_va & (arch::page_size - 1);
		const auto first = std::min<std::size_t>(entry.size(), arch::page_size - off);

		auto target = shadow::create(page, off, std::span(entry).first(first));
		if (!target)
			return std::nullopt;

		hook_record rec{ std::move(*target), {} };

		if (first < entry.size() && !handle_page_overflow(rec, page, std::span(entry).subspan(first)))
			return std::nullopt;

		return std::move(rec);
	}
}

bool hook::init() noexcept
{
	return arena::init();
}

bool hook::install(const std::uintptr_t target_va,
                   const std::span<const std::uint8_t> pre,
                   const std::span<const std::uint8_t> post,
                   const bool monitor) noexcept
{
	if (list.contains(target_va))
	{
		LOG_ERR("hook::install: already hooked");
		return false;
	}

	const auto orig_bytes = collect_orig_bytes(target_va, abs_jmp_size);
	if (!orig_bytes)
		return false;

	const auto stub = collect_stub_bytes(pre, *orig_bytes, post, target_va + orig_bytes->size(), monitor);

	const auto stub_va = arena::alloc(stub);
	if (!stub_va)
		return false;

	auto rec = redirect_entry(target_va, *stub_va);
	if (!rec)
	{
		arena::free(*stub_va, stub.size());
		return false;
	}

	rec->stub_va = *stub_va;
	rec->stub_size = stub.size();

	list.emplace(target_va, std::move(*rec));
	return true;
}

bool hook::remove(const std::uintptr_t target_va) noexcept
{
	const auto it = list.find(target_va);
	if (it == list.end())
	{
		LOG_ERR("hook::remove: not hooked");
		return false;
	}

	arena::free(it->second.stub_va, it->second.stub_size);
	list.erase(it);
	return true;
}
