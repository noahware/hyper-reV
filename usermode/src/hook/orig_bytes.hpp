#pragma once
#include "../arch/arch.hpp"
#include "../sys/kernel.hpp"
#include "../util/log.hpp"

#include <Zydis/Zydis.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace hook
{
	// a rel32 field that can only be filled in once the stub's final address is known
	struct fixup
	{
		std::size_t at;
		std::size_t end;
		std::uintptr_t target;
	};

	struct relocated
	{
		std::vector<std::uint8_t> bytes;
		std::vector<fixup> fixups;
		std::size_t consumed = 0;
	};

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

	// branches that exist only with an 8-bit displacement, so they can never be widened
	constexpr std::array<ZydisMnemonic, 6> no_rel32_form = {
		ZYDIS_MNEMONIC_JCXZ,
		ZYDIS_MNEMONIC_JECXZ,
		ZYDIS_MNEMONIC_JRCXZ,
		ZYDIS_MNEMONIC_LOOP,
		ZYDIS_MNEMONIC_LOOPE,
		ZYDIS_MNEMONIC_LOOPNE,
	};

	struct decoded
	{
		ZydisDecodedInstruction insn{};
		std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
	};

	inline std::optional<decoded> decode(const ZydisDecoder& decoder,
	                                     const std::span<const std::uint8_t> code) noexcept
	{
		ZydisDecoderContext ctx{};
		decoded out;

		if (ZYAN_FAILED(ZydisDecoderDecodeInstruction(&decoder, &ctx, code.data(), code.size(), &out.insn)))
			return std::nullopt;

		if (ZYAN_FAILED(ZydisDecoderDecodeOperands(&decoder, &ctx, &out.insn, out.operands.data(),
		                                           static_cast<ZyanU8>(out.operands.size()))))
			return std::nullopt;

		return out;
	}

	// where an instruction keeps the operand that makes it position dependent
	struct rel_field
	{
		std::uintptr_t target;
		std::uint8_t at;
		std::uint8_t bits;
	};

	inline std::optional<rel_field> find_rel_field(const decoded& d, const std::uintptr_t va) noexcept
	{
		for (std::size_t i = 0; i < d.insn.operand_count; ++i)
		{
			const auto& op = d.operands[i];

			const bool rip_mem = op.type == ZYDIS_OPERAND_TYPE_MEMORY && op.mem.base == ZYDIS_REGISTER_RIP;
			const bool rel_imm = op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && op.imm.is_relative;

			if (!rip_mem && !rel_imm)
				continue;

			std::uint64_t abs = 0;

			if (ZYAN_FAILED(ZydisCalcAbsoluteAddress(&d.insn, &op, va, &abs)))
				return std::nullopt;

			if (rip_mem)
				return rel_field{ abs, d.insn.raw.disp.offset, d.insn.raw.disp.size };

			return rel_field{ abs, d.insn.raw.imm[0].offset, d.insn.raw.imm[0].size };
		}

		return std::nullopt;
	}

	// re-encodes one position dependent instruction, widening an 8-bit branch so its
	// displacement can still reach the target from the pad page
	inline bool relocate_insn(const ZydisDecoder& decoder,
	                          const decoded& d,
	                          const rel_field& field,
	                          relocated& out) noexcept
	{
		const auto name = ZydisMnemonicGetString(d.insn.mnemonic);

		if (std::ranges::find(no_rel32_form, d.insn.mnemonic) != no_rel32_form.end())
		{
			LOG_ERR("hook: {} has no rel32 form, pick another target", name);
			return false;
		}

		ZydisEncoderRequest req{};

		if (ZYAN_FAILED(ZydisEncoderDecodedInstructionToEncoderRequest(&d.insn, d.operands.data(),
		                                                               d.insn.operand_count_visible, &req)))
		{
			LOG_ERR("hook: cannot build an encoder request for {}", name);
			return false;
		}

		if (field.bits < 32)
		{
			req.branch_type = ZYDIS_BRANCH_TYPE_NEAR;
			req.branch_width = ZYDIS_BRANCH_WIDTH_32;
		}

		std::array<std::uint8_t, ZYDIS_MAX_INSTRUCTION_LENGTH> buf{};
		ZyanUSize len = buf.size();

		if (ZYAN_FAILED(ZydisEncoderEncodeInstruction(&req, buf.data(), &len)))
		{
			LOG_ERR("hook: failed to encode {}", name);
			return false;
		}

		const auto emitted = std::span(buf).first(len);

		// ask the decoder where the re-encoded form put its displacement
		const auto moved = decode(decoder, emitted);
		const auto placed = moved ? find_rel_field(*moved, 0) : std::nullopt;

		if (!placed || placed->bits != 32)
		{
			LOG_ERR("hook: re-encoded {} has no rel32 to aim", name);
			return false;
		}

		const auto base = out.bytes.size();

		out.bytes.insert(out.bytes.end(), emitted.begin(), emitted.end());
		out.fixups.push_back({ base + placed->at, out.bytes.size(), field.target });
		return true;
	}

	// the bytes the entry patch will overwrite, rounded up to an instruction boundary and
	// rewritten so they still behave once they run from the pad page
	inline std::optional<relocated> collect_orig_bytes(const std::uintptr_t va,
	                                                   const std::size_t min_size) noexcept
	{
		const auto window = max_stub_bytes(va, min_size + arch::max_insn_size);
		if (!window)
			return std::nullopt;

		ZydisDecoder decoder;
		ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

		relocated out;

		while (out.consumed < min_size)
		{
			const auto at = std::span(*window).subspan(out.consumed);

			const auto d = decode(decoder, at);

			if (!d)
			{
				LOG_ERR("hook: decode failed at offset {}", out.consumed);
				return std::nullopt;
			}

			if (d->insn.attributes & ZYDIS_ATTRIB_IS_RELATIVE)
			{
				const auto field = find_rel_field(*d, va + out.consumed);

				if (!field)
				{
					LOG_ERR("hook: no relocatable operand on {} at offset {}",
					        ZydisMnemonicGetString(d->insn.mnemonic), out.consumed);
					return std::nullopt;
				}

				if (!relocate_insn(decoder, *d, *field, out))
					return std::nullopt;
			}
			else
			{
				const auto raw = at.first(d->insn.length);
				out.bytes.insert(out.bytes.end(), raw.begin(), raw.end());
			}

			out.consumed += d->insn.length;
		}

		return out;
	}

	// aims every rel32 at its target now that the stub sits at `stub_va`, with `base`
	// being where the relocated bytes start inside it
	inline bool resolve_fixups(const std::span<std::uint8_t> stub,
	                           const std::size_t base,
	                           const std::span<const fixup> fixups,
	                           const std::uintptr_t stub_va) noexcept
	{
		for (const auto& f : fixups)
		{
			const auto from = static_cast<std::int64_t>(stub_va + base + f.end);
			const auto delta = static_cast<std::int64_t>(f.target) - from;
			const auto rel = static_cast<std::int32_t>(delta);

			if (rel != delta)
			{
				LOG_ERR("hook: {:#x} is out of rel32 range from the stub", f.target);
				return false;
			}

			std::memcpy(stub.data() + base + f.at, &rel, sizeof(rel));
		}

		return true;
	}
}
