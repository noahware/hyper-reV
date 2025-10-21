#pragma once
#include <cstdint>

#define PAGE_SHIFT 12
#define PAGE_SIZE 0x1000
#define PAGE_MASK 0xFFF

#pragma warning(push)
#pragma warning(disable : 4201)

union virtual_address_t {
    std::uint64_t value;

    struct
    {
        std::uint64_t offset_4kb : 12;
        std::uint64_t pt_idx : 9;
        std::uint64_t pd_idx : 9;
        std::uint64_t pdpt_idx : 9;
        std::uint64_t pml4_idx : 9;
        std::uint64_t reserved : 16;
    };

    // 2mb Pages

    struct
    {
        std::uint64_t offset_2mb : 21;
        std::uint64_t pd_idx : 9;
        std::uint64_t pdpt_idx : 9;
        std::uint64_t pml4_idx : 9;
        std::uint64_t reserved : 16;
    };

    // 1Gb Pages

    struct
    {
        std::uint64_t offset_1gb : 30;
        std::uint64_t pdpt_idx : 9;
        std::uint64_t pml4_idx : 9;
        std::uint64_t reserved : 16;
    };
};

using gpa_t = virtual_address_t;
#pragma warning(pop)
