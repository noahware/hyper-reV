#include "hypercall/hypercall.hpp"
#include "util/log.hpp"

int main()
{
	LOG_INFO("hyper-reV");

	LOG_INFO("test hypercall: 0x{:X}", hc::test());

	return 0;
}
