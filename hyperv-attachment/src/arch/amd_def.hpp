#pragma once
#include <cstdint>
#include <ia32.hpp>

#pragma warning(push)
#pragma warning(disable: 4201)

enum class tlb_control : std::uint8_t
{
	do_not_flush = 0,
	flush_entire_tlb = 1,
	flush_guest_tlb_entries = 3,
	flush_guest_non_global_tlb_entries = 7,
};

union vmcb_clean
{
	std::uint32_t flags;

	struct
	{
		std::uint32_t i : 1;
		std::uint32_t iopm : 1;
		std::uint32_t asid : 1;
		std::uint32_t tpr : 1;
		std::uint32_t nested_paging : 1;
		std::uint32_t cr : 1;
		std::uint32_t dr : 1;
		std::uint32_t dt : 1;
		std::uint32_t seg : 1;
		std::uint32_t cr2 : 1;
		std::uint32_t lbr : 1;
		std::uint32_t avic : 1;
		std::uint32_t cet : 1;
		std::uint32_t reserved : 19;
	};
};

struct vmcb_control
{
	std::uint8_t pad_one[0xC];
	std::uint32_t intercept_misc_vector_3;
	std::uint8_t pad_two[0x4C];
	tlb_control tlb_control;
	std::uint8_t pad_three[0x13];
	std::uint64_t vmexit_reason;
	std::uint64_t first_exit_info;
	std::uint64_t second_exit_info;
	std::uint8_t pad_four[0x28];
	cr3 nested_cr3;
	std::uint8_t pad_five[0x8];
	vmcb_clean clean;
	std::uint8_t pad_six[0x4];
	std::uint64_t next_rip;
	std::uint8_t pad_seven[0x330];
};

struct vmcb_save_state
{
	std::uint8_t pad_one[0x150];
	std::uint64_t cr3;
	std::uint8_t pad_two[0x20];
	std::uint64_t rip;
	std::uint8_t pad_three[0x58];
	std::uint64_t rsp;
	std::uint8_t pad_four[0x18];
	std::uint64_t rax;
};

struct vmcb
{
	vmcb_control control;
	vmcb_save_state save_state;
};

union npf_exit_info
{
	std::uint64_t flags;

	struct
	{
		std::uint64_t present : 1;
		std::uint64_t write_access : 1;
		std::uint64_t user_access : 1;
		std::uint64_t npte_reserved_set : 1;
		std::uint64_t execute_access : 1;
		std::uint64_t reserved_one : 1;
		std::uint64_t shadow_stack_access : 1;
		std::uint64_t reserved_two : 25;
		std::uint64_t final_gpa_translation : 1;
		std::uint64_t gpt_translation : 1;
		std::uint64_t reserved_three : 3;
		std::uint64_t supervisor_shadow_stack : 1;
		std::uint64_t reserved_four : 26;
	};
};

#pragma warning(pop)

inline constexpr std::uint64_t SVM_EXIT_REASON_PHYSICAL_NMI = 0x61;
inline constexpr std::uint64_t SVM_EXIT_REASON_CPUID        = 0x72;
inline constexpr std::uint64_t SVM_EXIT_REASON_NPF          = 0x400;
