#include "hypercall/hypercall.hpp"
#include "util/log.hpp"

int main()
{
	LOG_INFO("hyper-reV");

	LOG_INFO("test hypercall: 0x{:X}", hc::test());
	LOG_INFO("guest cr3: 0x{:X}", hc::curr_cr3());

	return 0;
}
