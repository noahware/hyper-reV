#include "hypercall/hypercall.hpp"
#include "util/log.hpp"

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

	return 0;
}
