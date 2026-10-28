#pragma once
#include <cstdint>
#include <cstddef>

struct boot_info;

class vcpu
{
public:
	static void init(const boot_info& bi);

	static std::size_t count();
	static std::size_t curr_id();
	static bool is_bsp();
	static vcpu& curr();
};
