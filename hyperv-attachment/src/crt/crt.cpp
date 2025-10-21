#include "crt.h"
#include <intrin.h>

void crt::copy_memory(void *destination, const void *source, std::uint64_t size)
{
    __movsb(static_cast<std::uint8_t *>(destination), static_cast<const std::uint8_t *>(source), size);
}

void crt::set_memory(void *destination, std::uint8_t value, std::uint64_t size)
{
    __stosb(static_cast<std::uint8_t *>(destination), value, size);
}

crt::mutex_t::mutex_t() : value(0)
{
}

void crt::mutex_t::lock()
{
    while (_InterlockedCompareExchange64(&this->value, 1, 0) != 0)
    {
        _mm_pause();
    }
}

void crt::mutex_t::release()
{
    _InterlockedExchange64(&this->value, 0);
}

std::uint64_t *crt::bitmap_t::get_row(const std::uint64_t index) const
{
    if (this->value == nullptr)
    {
        return nullptr;
    }

    const std::uint64_t row_id = index / 64;

    if (this->value_count <= row_id)
    {
        return nullptr;
    }

    return &this->value[row_id];
}

void crt::bitmap_t::set_all() const
{
    if (this->value != nullptr)
    {
        for (std::uint64_t i = 0; i < this->value_count; i++)
        {
            std::uint64_t &row_state = this->value[i];

            row_state = UINT64_MAX;
        }
    }
}

void crt::bitmap_t::set(const std::uint64_t index) const
{
    std::uint64_t *row = this->get_row(index);

    if (row != nullptr)
    {
        const std::uint64_t bit = index % bit_count_in_row;

        *row |= 1ull << bit;
    }
}

void crt::bitmap_t::clear(const std::uint64_t index) const
{
    std::uint64_t *row = this->get_row(index);

    if (row != nullptr)
    {
        const std::uint64_t bit = index % bit_count_in_row;

        *row &= ~(1ull << bit);
    }
}

std::uint8_t crt::bitmap_t::is_set(const std::uint64_t index) const
{
    const std::uint64_t *row = this->get_row(index);

    if (row == nullptr)
    {
        return 0;
    }

    const std::uint64_t row_value = *row;
    const std::uint64_t bit = index % bit_count_in_row;

    return (row_value >> bit) & 1;
}

void crt::bitmap_t::set_map_value(std::uint64_t *const value)
{
    this->value = value;
}

void crt::bitmap_t::set_map_value_count(const std::uint64_t value_count)
{
    this->value_count = value_count;
}
