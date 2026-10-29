#pragma once
#include "../sys/kernel.hpp"
#include "../util/log.hpp"

#include <Zydis/Zydis.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace hook
{
	constexpr std::size_t max_insn_size = 15;

	inline std::optional<std::vector<std::uint8_t>> max_stub_bytes(const std::uintptr_t va,
	                                                               const std::size_t size) noexcept
	{
		std::vector<std::uint8_t> raw(size);

		if (!sys::kern::read(va, std::span(raw)))
		{
			LOG_ERR("hook: failed to read bytes at {:#x}", va);
			return std::nullopt;
		}

		return raw;
	}

	inline std::optional<ZydisDecodedInstruction> decode_relocatable(const ZydisDecoder& decoder,
	                                                                 const std::span<const std::uint8_t> code,
	                                                                 const std::size_t at) noexcept
	{
		ZydisDecodedInstruction insn;

		if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&decoder, nullptr, code.data(), code.size(), &insn)))
		{
			LOG_ERR("hook: decode failed at offset {}", at);
			return std::nullopt;
		}

		if (insn.attributes & ZYDIS_ATTRIB_IS_RELATIVE)
		{
			LOG_ERR("hook: relative insn at offset {}, pick another target", at);
			return std::nullopt;
		}

		return insn;
	}

	// the bytes the entry patch will overwrite, rounded up to an instruction boundary.
	// they get spliced into the stub so the original code still runs from the pad page.
	inline std::optional<std::vector<std::uint8_t>> collect_orig_bytes(const std::uintptr_t va,
	                                                                   const std::size_t min_size) noexcept
	{
		auto raw = max_stub_bytes(va, min_size + max_insn_size);
		if (!raw)
			return std::nullopt;

		ZydisDecoder decoder;
		ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

		std::size_t consumed = 0;

		while (consumed < min_size)
		{
			const auto insn = decode_relocatable(decoder, std::span(*raw).subspan(consumed), consumed);
			if (!insn)
				return std::nullopt;

			consumed += insn->length;
		}

		raw->resize(consumed);
		return raw;
	}
}
