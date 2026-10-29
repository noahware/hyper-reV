#include "hypercall/hypercall.hpp"
#include "util/log.hpp"
#include <Windows.h>
#include <cstring>

namespace
{
	constexpr std::size_t page_size = 4096;

	void test_hook() noexcept
	{
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

	// mov eax, imm32; ret
	void write_ret_stub(std::uint8_t* p, std::uint32_t val) noexcept
	{
		p[0] = 0xB8;
		std::memcpy(p + 1, &val, 4);
		p[5] = 0xC3;
	}

	void test_hook_exec() noexcept
	{
		constexpr std::uint32_t target_magic = 0xAAAAAAAA;
		constexpr std::uint32_t shadow_magic = 0xBBBBBBBB;

		auto* target = static_cast<std::uint8_t*>(
			VirtualAlloc(nullptr, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		auto* shadow = static_cast<std::uint8_t*>(
			VirtualAlloc(nullptr, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

		if (!target || !shadow)
		{
			LOG_ERR("test_hook_exec: VirtualAlloc failed: {}", GetLastError());
			return;
		}

		std::memset(target, 0xCC, page_size);
		std::memset(shadow, 0xCC, page_size);

		write_ret_stub(target, target_magic);
		write_ret_stub(shadow, shadow_magic);

		VirtualLock(target, page_size);
		VirtualLock(shadow, page_size);

		using stub_fn = std::uint32_t(*)();
		const auto call_target = reinterpret_cast<stub_fn>(target);

		const auto pre_hook = call_target();
		LOG_INFO("pre-hook exec:  0x{:X} (expect 0x{:X})", pre_hook, target_magic);

		if (pre_hook != target_magic)
		{
			LOG_ERR("FAIL: pre-hook returned wrong value");
			VirtualFree(shadow, 0, MEM_RELEASE);
			VirtualFree(target, 0, MEM_RELEASE);
			return;
		}

		const auto cr3 = hc::curr_cr3();
		const auto target_gpa = hc::translate_va(cr3, reinterpret_cast<std::uintptr_t>(target));
		const auto shadow_gpa = hc::translate_va(cr3, reinterpret_cast<std::uintptr_t>(shadow));

		if (!target_gpa || !shadow_gpa)
		{
			LOG_ERR("FAIL: VA->GPA translation failed");
			VirtualFree(shadow, 0, MEM_RELEASE);
			VirtualFree(target, 0, MEM_RELEASE);
			return;
		}

		if (!hc::hook_add(target_gpa, shadow_gpa))
		{
			LOG_ERR("FAIL: hook_add failed");
			VirtualFree(shadow, 0, MEM_RELEASE);
			VirtualFree(target, 0, MEM_RELEASE);
			return;
		}

		const auto hooked_exec = call_target();
		LOG_INFO("hooked exec:    0x{:X} (expect 0x{:X})", hooked_exec, shadow_magic);

		std::uint32_t read_imm = 0;
		std::memcpy(&read_imm, target + 1, 4);
		LOG_INFO("hooked read:    0x{:X} (expect 0x{:X})", read_imm, target_magic);

		hc::hook_remove(target_gpa);

		const auto post_hook = call_target();
		LOG_INFO("post-hook exec: 0x{:X} (expect 0x{:X})", post_hook, target_magic);

		const bool exec_ok = (hooked_exec == shadow_magic);
		const bool read_ok = (read_imm == target_magic);
		const bool restore_ok = (post_hook == target_magic);

		if (exec_ok && read_ok && restore_ok)
			LOG_INFO("PASS: hook exec/read split verified");
		else
		{
			if (!exec_ok) LOG_ERR("FAIL: execute did not redirect to shadow");
			if (!read_ok) LOG_ERR("FAIL: read did not return original bytes");
			if (!restore_ok) LOG_ERR("FAIL: unhook did not restore original exec");
		}

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
	test_hook_exec();

	return 0;
}
