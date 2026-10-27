#include "vcpu.hpp"
#include <boot_info.hpp>
#include <intrin.h>

namespace
{
	std::uint32_t curr_cpu_id_gs_off = 0;
	std::size_t cpu_count = 0;
}

void vcpu::init(const boot_info& bi)
{
	curr_cpu_id_gs_off = bi.curr_cpu_id_gs_off;
	cpu_count = bi.cpu_count;
}

std::size_t vcpu::count()
{
	return cpu_count;
}

std::size_t vcpu::curr_id()
{
	return __readgsdword(curr_cpu_id_gs_off);
}
