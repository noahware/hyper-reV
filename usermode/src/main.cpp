#include "hypercall/hypercall.hpp"
#include "util/log.hpp"
#include <Windows.h>
#include <cstring>

namespace
{
	void test_hook() noexcept
	{
		constexpr std::size_t page_size = 4096;

		auto* target = static_cast<std::uint8_t*>(
			VirtualAlloc(nullptr, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		auto* shadow = static_cast<std::uint8_t*>(
			VirtualAlloc(nullptr, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

		if (!target || !shadow)
		{
			LOG_ERR("VirtualAlloc failed: {}", GetLastError());
			return;
		}

		std::memset(target, 0xCC, page_size);
		std::memset(shadow, 0x90, page_size);

		VirtualLock(target, page_size);
		VirtualLock(shadow, page_size);

		const auto cr3 = hc::curr_cr3();

		const auto target_gpa = hc::translate_va(cr3, reinterpret_cast<std::uintptr_t>(target));
		const auto shadow_gpa = hc::translate_va(cr3, reinterpret_cast<std::uintptr_t>(shadow));

		LOG_INFO("hook target GPA: 0x{:X}", target_gpa);
		LOG_INFO("hook shadow GPA: 0x{:X}", shadow_gpa);

		if (!target_gpa || !shadow_gpa)
		{
			LOG_ERR("failed to translate VAs to GPAs");
			VirtualFree(shadow, 0, MEM_RELEASE);
			VirtualFree(target, 0, MEM_RELEASE);
			return;
		}

		const auto add_ok = hc::hook_add(target_gpa, shadow_gpa);
		LOG_INFO("hook_add: {}", add_ok);

		const auto remove_ok = hc::hook_remove(target_gpa);
		LOG_INFO("hook_remove: {}", remove_ok);

		const auto remove_again = hc::hook_remove(target_gpa);
		LOG_INFO("hook_remove (already removed): {}", remove_again);

		VirtualFree(shadow, 0, MEM_RELEASE);
		VirtualFree(target, 0, MEM_RELEASE);
	}
}

int main()
{
	LOG_INFO("hyper-reV");

	LOG_INFO("test hypercall: 0x{:X}", hc::test());

	const auto cr3 = hc::curr_cr3();
	LOG_INFO("guest cr3: 0x{:X}", cr3);

	int test_val = 0x41424344;
	const auto test_pa = hc::translate_va(cr3, reinterpret_cast<std::uintptr_t>(&test_val));
	LOG_INFO("translate_va(&test_val): 0x{:X}", test_pa);

	int read_val = 0;
	const auto read_pa = hc::translate_va(cr3, reinterpret_cast<std::uintptr_t>(&read_val));
	const auto phys_ok = hc::phys_copy(test_pa, read_pa, sizeof(read_val));
	LOG_INFO("phys_copy: {} val=0x{:X}", phys_ok, read_val);

	int virt_val = 0;
	const auto virt_ok = hc::virt_copy(cr3, reinterpret_cast<std::uintptr_t>(&test_val),
	                                    cr3, reinterpret_cast<std::uintptr_t>(&virt_val),
	                                    sizeof(virt_val));
	LOG_INFO("virt_copy: {} val=0x{:X}", virt_ok, virt_val);

	test_hook();

	return 0;
}
