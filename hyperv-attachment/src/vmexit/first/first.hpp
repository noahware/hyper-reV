#pragma once

struct boot_info;

namespace vmexit::first
{
	void init(const boot_info& bi);

	void handle();
	void handle_non_bsp();
}
