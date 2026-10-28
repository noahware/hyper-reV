#include "vcpu.hpp"
#include <boot_info.hpp>
#include <vector.hpp>
#include <intrin.h>

namespace
{
	std::uint32_t curr_cpu_id_gs_off = 0;
	cstd::vector<vcpu> vcpus;
}

void vcpu::init(const boot_info& bi)
{
	curr_cpu_id_gs_off = bi.curr_cpu_id_gs_off;
	vcpus.resize(bi.cpu_count);
}

std::size_t vcpu::count()
{
	return vcpus.size();
}

std::size_t vcpu::curr_id()
{
	return __readgsdword(curr_cpu_id_gs_off);
}

bool vcpu::is_bsp()
{
	constexpr std::size_t bsp_id = 0;

	return curr_id() == bsp_id;
}

vcpu& vcpu::curr()
{
	return vcpus[curr_id()];
}

vcpu& vcpu::get(const std::size_t id)
{
	return vcpus[id];
}
