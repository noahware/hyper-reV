#include "hook.h"
#include "kernel_detour_holder.h"
#include "../system/system.h"
#include "../hypercall/hypercall.h"
#include "../logs/logs.h"

#include <Windows.h>
#include <vector>
#include <array>
#include <memory_resource>

#include "hook_disassembly.h"

namespace
{
	std::unordered_map<std::uint64_t, hook::kernel_hook_info_t> kernel_hook_list = {};

	std::uint64_t kernel_detour_holder_base = 0;
	std::uint64_t kernel_detour_holder_physical_page = 0;
	std::uint8_t* kernel_detour_holder_shadow_page_mapped = nullptr;

	constexpr std::size_t inline_hook_bytes_size = 14;
}

std::uint8_t hook::set_up()
{
	kernel_detour_holder_shadow_page_mapped = static_cast<std::uint8_t*>(sys::user::allocate_locked_memory(
		page_size, PAGE_READWRITE));

	if (kernel_detour_holder_shadow_page_mapped == nullptr)
	{
		return 0;
	}

	const std::uint64_t shadow_page_physical = hypercall::translate_guest_virtual_address(
		reinterpret_cast<std::uint64_t>(kernel_detour_holder_shadow_page_mapped), sys::current_cr3);

	if (shadow_page_physical == 0)
	{
		return 0;
	}

	kernel_detour_holder_physical_page = hypercall::translate_guest_virtual_address(
		kernel_detour_holder_base, sys::current_cr3);

	if (kernel_detour_holder_physical_page == 0)
	{
		return 0;
	}

	// in case of a previously wrongfully ended session which would've left the hook still applied
	hypercall::remove_slat_code_hook(kernel_detour_holder_physical_page);

	hypercall::read_guest_physical_memory(kernel_detour_holder_shadow_page_mapped, kernel_detour_holder_physical_page,
	                                      page_size);

	const std::uint64_t hook_status = hypercall::add_slat_code_hook(kernel_detour_holder_physical_page,
	                                                                shadow_page_physical);

	if (hook_status == 0)
	{
		return 0;
	}

	kernel_detour_holder::set_up(reinterpret_cast<std::uint64_t>(kernel_detour_holder_shadow_page_mapped), page_size);

	return 1;
}

void hook::clean_up()
{
	for (const auto& [virtual_address, info] : kernel_hook_list)
	{
		remove_kernel_hook(virtual_address, 0);
	}

	kernel_hook_list.clear();

	if (kernel_detour_holder_physical_page != 0)
	{
		hypercall::remove_slat_code_hook(kernel_detour_holder_physical_page);
	}
}

void hook::set_kernel_detour_holder_base(const std::uint64_t address)
{
	kernel_detour_holder_base = address;
}

static std::pair<std::vector<std::uint8_t>, std::uint64_t> load_original_bytes_into_shadow_page(
	std::uint8_t* shadow_page_virtual, const std::uint64_t routine_to_hook_virtual, const std::uint8_t is_overflow_hook,
	const std::uint64_t extra_asm_byte_count)
{
	const std::uint64_t page_offset = routine_to_hook_virtual & 0xFFF;

	hypercall::read_guest_virtual_memory(shadow_page_virtual, routine_to_hook_virtual - page_offset, sys::current_cr3,
	                                     is_overflow_hook == 1 ? (page_size * 2) : page_size);

	return hook_disasm::get_routine_aligned_bytes(shadow_page_virtual + page_offset,
	                                              inline_hook_bytes_size + extra_asm_byte_count,
	                                              routine_to_hook_virtual);
}

static std::uint8_t set_up_inline_hook(std::uint8_t* shadow_page_virtual, const std::uint64_t routine_to_hook_virtual,
                                       const std::uint64_t routine_to_hook_physical, const std::uint64_t detour_address,
                                       const std::pair<std::vector<std::uint8_t>, std::uint64_t>& original_bytes,
                                       const std::span<const std::uint8_t> extra_assembled_bytes,
                                       const std::span<const std::uint8_t> post_original_assembled_bytes,
                                       const std::uint8_t is_overflow_hook,
                                       std::uint64_t& overflow_shadow_page_physical_address,
                                       std::uint64_t& overflow_page_physical_address)
{
	std::array<std::uint8_t, inline_hook_bytes_size> jmp_to_detour_bytes = {
		0x68, 0x21, 0x43, 0x65, 0x87, // push   0xffffffff87654321
		0xC7, 0x44, 0x24, 0x04, 0x78, 0x56, 0x34, 0x12, // mov    DWORD PTR [rsp+0x4],0x12345678
		0xC3 // ret
	};

	const parted_address_t parted_subroutine_to_jmp_to = {.value = detour_address};

	*reinterpret_cast<std::uint32_t*>(&jmp_to_detour_bytes[1]) = parted_subroutine_to_jmp_to.u.low_part;
	*reinterpret_cast<std::uint32_t*>(&jmp_to_detour_bytes[9]) = parted_subroutine_to_jmp_to.u.high_part;

	std::vector<std::uint8_t> inline_hook_bytes(extra_assembled_bytes.begin(), extra_assembled_bytes.end());

	inline_hook_bytes.insert(inline_hook_bytes.end(), jmp_to_detour_bytes.begin(), jmp_to_detour_bytes.end());

	if (static_cast<int>(post_original_assembled_bytes.empty()) == 0)
	{
		inline_hook_bytes.insert(inline_hook_bytes.end(), post_original_assembled_bytes.begin(),
		                         post_original_assembled_bytes.end());

		const std::uint64_t nop_bytes_needed = original_bytes.second - inline_hook_bytes.size();

		inline_hook_bytes.insert(inline_hook_bytes.end(), nop_bytes_needed, 0x90);
		// nop padding until next actual instruction
	}

	const std::uint64_t page_offset = routine_to_hook_physical & 0xFFF;

	if (is_overflow_hook == 1)
	{
		std::uint8_t* overflow_shadow_page_virtual = shadow_page_virtual + page_size;

		overflow_shadow_page_physical_address = hypercall::translate_guest_virtual_address(
			reinterpret_cast<std::uint64_t>(overflow_shadow_page_virtual), sys::current_cr3);

		if (overflow_shadow_page_physical_address == 0)
		{
			return 0;
		}

		const std::uint64_t overflow_page_virtual_address = routine_to_hook_virtual + page_size;

		overflow_page_physical_address = hypercall::translate_guest_virtual_address(
			overflow_page_virtual_address, sys::current_cr3);

		if (overflow_page_physical_address == 0)
		{
			return 0;
		}

		const std::uint64_t hook_end = page_offset + inline_hook_bytes.size();
		const std::uint64_t bytes_overflowed = hook_end - page_size;

		const std::uint64_t prior_page_copy_size = inline_hook_bytes.size() - bytes_overflowed;

		memcpy(shadow_page_virtual + page_offset, inline_hook_bytes.data(), prior_page_copy_size);
		memcpy(overflow_shadow_page_virtual, inline_hook_bytes.data() + prior_page_copy_size, bytes_overflowed);
	}
	else
	{
		memcpy(shadow_page_virtual + page_offset, inline_hook_bytes.data(), inline_hook_bytes.size());
	}

	return 1;
}

static std::uint8_t set_up_hook_handler(const std::uint64_t routine_to_hook_virtual,
                                        std::uint16_t& detour_holder_shadow_offset,
                                        const std::pair<std::vector<std::uint8_t>, std::uint64_t>& original_bytes,
                                        const std::span<const std::uint8_t> extra_assembled_bytes,
                                        const std::span<const std::uint8_t> post_original_assembled_bytes)
{
	std::array<std::uint8_t, 14> return_to_original_bytes = {
		0x68, 0x21, 0x43, 0x65, 0x87, // push   0xffffffff87654321
		0xC7, 0x44, 0x24, 0x04, 0x78, 0x56, 0x34, 0x12, // mov    DWORD PTR [rsp+0x4],0x12345678
		0xC3 // ret
	};

	parted_address_t parted_subroutine_to_jmp_to{};

	if (static_cast<int>(post_original_assembled_bytes.empty()) == 0)
	{
		parted_subroutine_to_jmp_to.value = routine_to_hook_virtual + extra_assembled_bytes.size() +
			inline_hook_bytes_size;
	}
	else
	{
		parted_subroutine_to_jmp_to.value = routine_to_hook_virtual + original_bytes.second;
	}

	*reinterpret_cast<std::uint32_t*>(&return_to_original_bytes[1]) = parted_subroutine_to_jmp_to.u.low_part;
	*reinterpret_cast<std::uint32_t*>(&return_to_original_bytes[9]) = parted_subroutine_to_jmp_to.u.high_part;

	std::vector<std::uint8_t> hook_handler_bytes = original_bytes.first;

	hook_handler_bytes.insert(hook_handler_bytes.end(), return_to_original_bytes.begin(),
	                          return_to_original_bytes.end());

	void* bytes_buffer = kernel_detour_holder::allocate_memory(static_cast<std::uint16_t>(hook_handler_bytes.size()));

	if (bytes_buffer == nullptr)
	{
		return 0;
	}

	detour_holder_shadow_offset = kernel_detour_holder::get_allocation_offset(bytes_buffer);

	memcpy(bytes_buffer, hook_handler_bytes.data(), hook_handler_bytes.size());

	return 1;
}

std::uint8_t hook::add_kernel_hook(const std::uint64_t routine_to_hook_virtual,
                                   const std::span<const std::uint8_t> extra_assembled_bytes,
                                   const std::span<const std::uint8_t> post_original_assembled_bytes)
{
	if (kernel_detour_holder_physical_page == 0 && set_up() == 0)
	{
		LOG_ERR("unable to set up kernel hook helper");

		return 0;
	}

	if (kernel_hook_list.contains(routine_to_hook_virtual))
	{
		return 0;
	}

	const std::uint64_t routine_to_hook_physical = hypercall::translate_guest_virtual_address(
		routine_to_hook_virtual, sys::current_cr3);

	if (routine_to_hook_physical == 0)
	{
		return 0;
	}

	const std::uint64_t page_offset = routine_to_hook_physical & 0xFFF;
	const std::uint64_t hook_end = page_offset + inline_hook_bytes_size + extra_assembled_bytes.size();

	const std::uint8_t is_overflow_hook = static_cast<const std::uint8_t>(page_size < hook_end);

	void* shadow_page_virtual = sys::user::allocate_locked_memory(is_overflow_hook == 1 ? (page_size * 2) : page_size,
	                                                              PAGE_READWRITE);

	if (shadow_page_virtual == nullptr)
	{
		return 0;
	}

	const std::uint64_t shadow_page_physical = hypercall::translate_guest_virtual_address(
		reinterpret_cast<std::uint64_t>(shadow_page_virtual), sys::current_cr3);

	if (shadow_page_physical == 0)
	{
		return 0;
	}

	const auto original_bytes = load_original_bytes_into_shadow_page(static_cast<std::uint8_t*>(shadow_page_virtual),
	                                                                 routine_to_hook_virtual, is_overflow_hook,
	                                                                 extra_assembled_bytes.size() +
	                                                                 post_original_assembled_bytes.size());

	if (original_bytes.first.empty())
	{
		return 0;
	}

	std::uint16_t detour_holder_shadow_offset = 0;

	const std::uint8_t status = set_up_hook_handler(routine_to_hook_virtual, detour_holder_shadow_offset,
	                                                original_bytes, extra_assembled_bytes,
	                                                post_original_assembled_bytes);

	if (status == 0)
	{
		return 0;
	}

	const std::uint64_t detour_address = kernel_detour_holder_base + detour_holder_shadow_offset;

	std::uint64_t overflow_shadow_page_physical_address = 0;
	std::uint64_t overflow_page_physical_address = 0;

	std::uint64_t hook_status = set_up_inline_hook(static_cast<std::uint8_t*>(shadow_page_virtual),
	                                               routine_to_hook_virtual, routine_to_hook_physical, detour_address,
	                                               original_bytes, extra_assembled_bytes, post_original_assembled_bytes,
	                                               is_overflow_hook, overflow_shadow_page_physical_address,
	                                               overflow_page_physical_address);

	if (hook_status == 0)
	{
		return 0;
	}

	hook_status = hypercall::add_slat_code_hook(routine_to_hook_physical, shadow_page_physical);

	if (hook_status == 0)
	{
		return 0;
	}

	if (overflow_shadow_page_physical_address != 0)
	{
		hook_status = hypercall::add_slat_code_hook(overflow_page_physical_address,
		                                            overflow_shadow_page_physical_address);

		if (hook_status == 0)
		{
			return 0;
		}
	}

	const kernel_hook_info_t hook_info(routine_to_hook_physical >> 12, overflow_page_physical_address >> 12,
	                                   shadow_page_virtual, detour_holder_shadow_offset);

	kernel_hook_list[routine_to_hook_virtual] = hook_info;

	return 1;
}

std::uint8_t hook::remove_kernel_hook(const std::uint64_t hooked_routine_virtual, const std::uint8_t do_list_erase)
{
	if (!kernel_hook_list.contains(hooked_routine_virtual))
	{
		LOG_ERR("unable to find kernel hook");

		return 0;
	}

	const kernel_hook_info_t hook_info = kernel_hook_list[hooked_routine_virtual];

	if (hypercall::remove_slat_code_hook(hook_info.original_page_pfn() << 12) == 0)
	{
		LOG_ERR("unable to remove slat counterpart of kernel hook");

		return 0;
	}

	if (hook_info.overflow_page_pfn() != 0 && hypercall::remove_slat_code_hook(hook_info.overflow_page_pfn() << 12) ==
		0)
	{
		LOG_ERR("unable to remove slat counterpart of kernel hook (2)");

		return 0;
	}

	if (do_list_erase == 1)
	{
		kernel_hook_list.erase(hooked_routine_virtual);
	}

	void* detour_holder_allocation = kernel_detour_holder::get_allocation_from_offset(
		hook_info.detour_holder_shadow_offset());

	kernel_detour_holder::free_memory(detour_holder_allocation);

	if (sys::user::free_memory(hook_info.mapped_shadow_page()) == 0)
	{
		LOG_ERR("unable to deallocate mapped shadow page");

		return 0;
	}

	return 1;
}

std::uint64_t hook::kernel_hook_info_t::original_page_pfn() const
{
	return original_page_pfn_;
}

void hook::kernel_hook_info_t::set_original_page_pfn(const std::uint64_t original_page_pfn)
{
	original_page_pfn_ = original_page_pfn;
}

std::uint64_t hook::kernel_hook_info_t::overflow_page_pfn() const
{
	return overflow_page_pfn_;
}

void hook::kernel_hook_info_t::set_overflow_page_pfn(const std::uint64_t overflow_page_pfn)
{
	overflow_page_pfn_ = overflow_page_pfn;
}

void* hook::kernel_hook_info_t::mapped_shadow_page() const
{
	return reinterpret_cast<void*>(mapped_shadow_page_);
}

void hook::kernel_hook_info_t::set_mapped_shadow_page(void* const mapped_shadow_page)
{
	mapped_shadow_page_ = reinterpret_cast<std::uint64_t>(mapped_shadow_page);
}

std::uint16_t hook::kernel_hook_info_t::detour_holder_shadow_offset() const
{
	return detour_holder_shadow_offset_;
}

void hook::kernel_hook_info_t::set_detour_holder_shadow_offset(const std::uint64_t detour_holder_shadow_offset)
{
	detour_holder_shadow_offset_ = detour_holder_shadow_offset;
}
