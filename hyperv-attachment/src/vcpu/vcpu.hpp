#pragma once
#include <cstdint>
#include <cstddef>

class vcpu
{
public:
	vcpu();

	static std::size_t count();
	static std::size_t curr_id();
	static vcpu& curr();
};
