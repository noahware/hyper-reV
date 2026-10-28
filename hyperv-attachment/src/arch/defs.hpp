#pragma once
#include <cstdint>
#include <cstddef>

using phys_addr = std::uint64_t;

#pragma warning(push)
#pragma warning(disable: 4201)
union virt_addr
{
	std::uint64_t val;

	struct
	{
		std::uint64_t off : 12;
		std::uint64_t pte : 9;
		std::uint64_t pde : 9;
		std::uint64_t pdpte : 9;
		std::uint64_t pml4e : 9;
		std::uint64_t reserved : 16;
	};
};
#pragma warning(pop)

enum class page_perm : std::uint8_t
{
	none    = 0,
	read    = 1 << 0,
	write   = 1 << 1,
	execute = 1 << 2,
	rw      = read | write,
	rx      = read | execute,
	rwx     = read | write | execute,
};

constexpr page_perm operator&(const page_perm a, const page_perm b)
{
	return static_cast<page_perm>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

constexpr page_perm operator|(const page_perm a, const page_perm b)
{
	return static_cast<page_perm>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

constexpr page_perm& operator|=(page_perm& a, const page_perm b)
{
	return a = a | b;
}

struct slat_violation
{
	phys_addr gpa;
	page_perm access;
	bool caused_by_translation;
};

inline constexpr std::size_t page_shift = 12;
inline constexpr std::size_t large_page_shift = 21;
inline constexpr std::size_t large_page_1gb_shift = 30;
inline constexpr std::size_t page_size = 0x1000;
inline constexpr std::size_t page_mask = page_size - 1;
inline constexpr std::size_t paging_entry_count = 512;
