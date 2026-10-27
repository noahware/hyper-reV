#pragma once
#include <cstdint>
#include <cstddef>

struct boot_info;

class vcpu
{
public:
	vcpu();

	static void init(const boot_info& bi);

	static std::size_t count();
	static std::size_t curr_id();
	static vcpu& curr();
};
