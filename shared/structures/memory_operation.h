#pragma once
#include <cstdint>

constexpr std::uint64_t translation_error_mask = 1ull << 63;

enum class translation_result_t : std::uint8_t
{
    pml4e_not_present,
    pdpte_not_present,
    pde_not_present,
    pte_not_present,
    success
};

enum class memory_operation_t : std::uint64_t
{
    read_operation,
    write_operation
};

inline bool is_translation_error(std::uint64_t value)
{
    return (value & (1ull << 63)) != 0;
}

inline std::uint64_t make_translation_error(translation_result_t result)
{
    return translation_error_mask | static_cast<std::uint64_t>(result);
}

inline translation_result_t get_translation_result(std::uint64_t value)
{
    return static_cast<translation_result_t>(value & 0xFF);
}