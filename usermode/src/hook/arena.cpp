#include "arena.hpp"
#include "shadow.hpp"

#include "../arch/arch.hpp"
#include "../sys/dump.hpp"
#include "../sys/kernel.hpp"
#include "../util/log.hpp"

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

namespace
{
	hook::shadow pad;
	std::uintptr_t pad_va = 0;

	struct hole
	{
		std::size_t off;
		std::size_t len;
	};

	// sorted by offset, never adjacent to each other
	std::vector<hole> holes;

	const pe::section_header* find_pad_section(const pe::image* const img) noexcept
	{
		for (const auto& s : img->sections())
		{
			if (s.characteristics.mem_execute && s.name().contains("Pad"))
				return &s;
		}

		return nullptr;
	}
}

bool hook::arena::init() noexcept
{
	const auto ntos = sys::kern::nt();
	if (ntos == sys::kern::modules_list.end())
	{
		LOG_ERR("hook::arena: no ntoskrnl");
		return false;
	}

	const auto image = sys::dump::run(ntos->second.base);
	if (!image)
	{
		LOG_ERR("hook::arena: dump failed");
		return false;
	}

	const auto* const section = find_pad_section(image.img());
	if (!section)
	{
		LOG_ERR("hook::arena: no Pad section");
		return false;
	}

	const auto va = ntos->second.base + section->virtual_address;

	auto mapped = shadow::create(va, 0, {});
	if (!mapped)
		return false;

	pad = std::move(*mapped);
	pad_va = va;
	holes = { { 0, std::min<std::size_t>(arch::page_size, section->virtual_size) } };
	return true;
}

std::optional<hook::arena::slot> hook::arena::alloc(const std::size_t size) noexcept
{
	for (auto it = holes.begin(); it != holes.end(); ++it)
	{
		if (it->len < size)
			continue;

		const auto off = it->off;

		it->off += size;
		it->len -= size;

		if (it->len == 0)
			holes.erase(it);

		return slot{ pad_va + off, { pad.data() + off, size } };
	}

	LOG_ERR("hook::arena: no slot for {} bytes", size);
	return std::nullopt;
}

void hook::arena::free(const std::uintptr_t va, const std::size_t size) noexcept
{
	const hole freed{ va - pad_va, size };

	auto it = holes.insert(std::ranges::upper_bound(holes, freed.off, {}, &hole::off), freed);

	if (const auto next = it + 1; next != holes.end() && it->off + it->len == next->off)
	{
		it->len += next->len;
		holes.erase(next);
	}

	if (it != holes.begin())
	{
		const auto prev = it - 1;

		if (prev->off + prev->len == it->off)
		{
			prev->len += it->len;
			holes.erase(it);
		}
	}
}
