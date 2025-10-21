#pragma once
#include <ia32-doc/ia32.hpp>
#include "../../shared/structures/memory_operation.h"
#include "../structures/virtual_address.h"

#ifdef _INTELMACHINE
extern "C" void invalidate_ept_mappings(invept_type type, const invept_descriptor &descriptor);
#endif

namespace slat
{
void set_up();
void process_first_vmexit();

std::uint8_t try_hide_heap_pages(std::uint64_t heap_physical_address, std::uint64_t heap_physical_end);

cr3 get_cr3();

void flush_current_logical_processor_slat_cache(std::uint8_t has_slat_cr3_changed = 0);
void flush_all_logical_processors_slat_cache();

std::uint64_t translate_guest_physical_address(cr3 slat_cr3, virtual_address_t guest_physical_address,
                                               translation_result_t &result,
                                               std::uint64_t *size_left_of_slat_page = nullptr);
std::uint64_t add_slat_code_hook(cr3 slat_cr3, virtual_address_t guest_physical_address,
                                 virtual_address_t shadow_page_guest_physical_address);
std::uint64_t remove_slat_code_hook(cr3 slat_cr3, virtual_address_t guest_physical_address);
std::uint64_t hide_physical_page_from_guest(cr3 slat_cr3, virtual_address_t guest_physical_address);

std::uint8_t process_slat_violation();

class hook_entry_t
{
  protected:
    std::uint64_t next : 48;
    std::uint64_t original_read_access : 1;
    std::uint64_t original_write_access : 1;
    std::uint64_t original_execute_access : 1;
    std::uint64_t paging_split_state : 1;
    std::uint64_t reserved : 4;

    std::uint64_t higher_original_pfn : 4;
    std::uint64_t higher_shadow_pfn : 4;

    std::uint32_t lower_original_pfn;
    std::uint32_t lower_shadow_pfn;

  public:
    hook_entry_t *get_next() const;
    void set_next(hook_entry_t *next_entry);

    std::uint64_t get_original_pfn() const;
    std::uint64_t get_shadow_pfn() const;

    std::uint64_t get_original_read_access() const;
    std::uint64_t get_original_write_access() const;
    std::uint64_t get_original_execute_access() const;
    std::uint64_t get_paging_split_state() const;

    void set_original_pfn(std::uint64_t original_pfn);
    void set_shadow_pfn(std::uint64_t shadow_pfn);

    void set_original_read_access(std::uint64_t original_read_access_in);
    void set_original_write_access(std::uint64_t original_write_access_in);
    void set_original_execute_access(std::uint64_t original_execute_access_in);
    void set_paging_split_state(std::uint64_t paging_split_state_in);

    static hook_entry_t *find(hook_entry_t *list_head, std::uint64_t target_original_4kb_pfn,
                              hook_entry_t **previous_entry_out = nullptr);
    static hook_entry_t *find_in_2mb_range(hook_entry_t *list_head, std::uint64_t target_original_4kb_pfn,
                                           hook_entry_t *excluding_hook = nullptr);
    static hook_entry_t *find_closest_in_2mb_range(hook_entry_t *list_head, std::uint64_t target_original_4kb_pfn,
                                                   hook_entry_t *excluding_hook = nullptr);
};

inline hook_entry_t *available_hook_list_head = nullptr;
inline hook_entry_t *used_hook_list_head = nullptr;
} // namespace slat
