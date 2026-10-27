#include "vcpu.hpp"
#include <boot_info.hpp>
#include <ia32.hpp>
#include <intrin.h>

namespace
{
	std::uint32_t curr_cpu_id_gs_off = 0;

	// not in ia32-doc, same register layout as CPUID_EXTENDED_TOPOLOGY (SDM vol 1, table 21-70)
	constexpr std::uint32_t cpuid_v2_extended_topology = 0x1F;

	// returns the shift of the highest topology level, or 0 if the leaf is unsupported or invalid
	std::uint32_t get_topology_leaf_shift(const std::uint32_t leaf)
	{
		std::uint32_t shift = 0;

		// sub-leaves are enumerated until one returns 0 in ebx[15:0], sub-leaf 0 doing so means the leaf is invalid
		for (std::uint32_t sub_leaf = 0; ; sub_leaf++)
		{
			cpuid_eax_0b level = { };

			__cpuidex(reinterpret_cast<int*>(&level), static_cast<int>(leaf), static_cast<int>(sub_leaf));

			if (level.ebx.number_of_logical_processors_at_this_level_type == 0)
			{
				break;
			}

			// bits to shift the x2apic id right by to address the next higher domain, i.e. it spans every id below it
			shift = level.eax.x2apic_id_to_unique_topology_id_shift;
		}

		return shift;
	}

	// size of the apic id space reserved for this package
	std::uint32_t get_apic_id_count()
	{
		cpuid_eax_00 max_leaf = { };

		__cpuid(reinterpret_cast<int*>(&max_leaf), CPUID_SIGNATURE);

		// 1fh is the preferred superset of 0bh, and querying a leaf above the max returns data for the highest basic leaf
		std::uint32_t shift = 0;

		if (max_leaf.max_cpuid_input_value >= cpuid_v2_extended_topology)
		{
			shift = get_topology_leaf_shift(cpuid_v2_extended_topology);
		}

		if (shift == 0 && max_leaf.max_cpuid_input_value >= CPUID_EXTENDED_TOPOLOGY)
		{
			shift = get_topology_leaf_shift(CPUID_EXTENDED_TOPOLOGY);
		}

		if (shift != 0)
		{
			return 1u << shift;
		}

		// no x2apic topology leaf, fall back to the legacy max addressable ids (only valid with htt set)
		cpuid_eax_01 version = { };

		__cpuid(reinterpret_cast<int*>(&version), CPUID_VERSION_INFORMATION);

		if (version.cpuid_feature_information_edx.hyper_threading_technology == 0)
		{
			return 1;
		}

		// the id space is the max addressable ids rounded up to a power of 2
		std::uint32_t id_count = 1;

		while (id_count < version.cpuid_additional_information.max_addressable_ids)
		{
			id_count <<= 1;
		}

		return id_count;
	}
}

void vcpu::init(const boot_info& bi)
{
	curr_cpu_id_gs_off = bi.curr_cpu_id_gs_off;
}

std::size_t vcpu::count()
{
	static const std::size_t count = get_apic_id_count();

	return count;
}

std::size_t vcpu::curr_id()
{
	return __readgsdword(curr_cpu_id_gs_off);
}
