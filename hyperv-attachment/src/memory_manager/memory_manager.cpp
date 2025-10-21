#include "memory_manager.h"

#include "../crt/crt.h"
#include "../slat/slat.h"

constexpr std::uint64_t host_physical_memory_access_base = 255ull << 39;

std::uint64_t memory_manager::map_host_physical(std::uint64_t host_physical_address)
{
    return host_physical_memory_access_base + host_physical_address;
}

std::uint64_t memory_manager::unmap_host_physical(std::uint64_t host_mapped_address)
{
    return host_mapped_address - host_physical_memory_access_base;
}

std::uint64_t memory_manager::map_guest_physical(cr3 slat_cr3, std::uint64_t guest_physical_address,
                                                 std::uint64_t *size_left_of_ept_page)
{
    const std::uint64_t host_physical_address = slat::translate_guest_physical_address(
        slat_cr3, virtual_address_t{.value = guest_physical_address}, size_left_of_ept_page);
    if (host_physical_address == NULL)
    {
        return 0;
    }
    return host_physical_memory_access_base + host_physical_address;
}

std::uint64_t memory_manager::translate_guest_virtual_address(cr3 guest_cr3, cr3 slat_cr3,
                                                              virtual_address_t guest_virtual_address,
                                                              std::uint64_t *size_left_of_page)
{
    pml4e_64 *pml4 =
        reinterpret_cast<pml4e_64 *>(map_guest_physical(slat_cr3, guest_cr3.address_of_page_directory << 12));
    if (!pml4)
    {
        return 0;
    }

    pml4e_64 pml4e = pml4[guest_virtual_address.pml4_idx];
    if (pml4e.present == 0 || pml4e.page_frame_number == 0)
    {
        return 0;
    }

    pdpte_64 *pdpt = reinterpret_cast<pdpte_64 *>(map_guest_physical(slat_cr3, pml4e.page_frame_number << 12));
    if (!pdpt)
    {
        return 0;
    }

    pdpte_64 pdpte = pdpt[guest_virtual_address.pdpt_idx];
    if (pdpte.present == 0 || pdpte.page_frame_number == 0)
    {
        return 0;
    }

    // 1GB pages
    if (pdpte.large_page == 1)
    {
        pdpte_1gb_64 large_pdpte = {.flags = pdpte.flags};
        const std::uint64_t page_offset = guest_virtual_address.offset_1gb;

        if (size_left_of_page != nullptr)
        {
            *size_left_of_page = (1ull << 30) - page_offset;
        }

        return (large_pdpte.page_frame_number << 30) + page_offset;
    }

    pde_64 *pd = reinterpret_cast<pde_64 *>(map_guest_physical(slat_cr3, pdpte.page_frame_number << 12));
    if (!pd)
    {
        return 0;
    }

    pde_64 pde = pd[guest_virtual_address.pd_idx];
    if (pde.present == 0 || pde.page_frame_number == 0)
    {
        return 0;
    }

    // 2MB pages
    if (pde.large_page == 1)
    {
        pde_2mb_64 large_pde = {.flags = pde.flags};
        const std::uint64_t page_offset = guest_virtual_address.offset_2mb;

        if (size_left_of_page != nullptr)
        {
            *size_left_of_page = (1ull << 21) - page_offset;
        }

        return (large_pde.page_frame_number << 21) + page_offset;
    }

    pte_64 *pt = reinterpret_cast<pte_64 *>(map_guest_physical(slat_cr3, pde.page_frame_number << 12));
    if (!pt)
    {
        return 0;
    }

    pte_64 pte = pt[guest_virtual_address.pt_idx];
    if (pte.present == 0 || pte.page_frame_number == 0)
    {
        return 0;
    }

    // 4KB pages
    const std::uint64_t page_offset = guest_virtual_address.offset_4kb;

    if (size_left_of_page != nullptr)
    {
        *size_left_of_page = (1ull << 12) - page_offset;
    }

    return (pte.page_frame_number << 12) + page_offset;
}

std::uint64_t memory_manager::translate_host_virtual_address(cr3 host_cr3, virtual_address_t host_virtual_address,
                                                             std::uint64_t *size_left_of_page)
{

    auto physPml4 = (host_cr3.address_of_page_directory << 12);
    pml4e_64 *pml4 = reinterpret_cast<pml4e_64 *>(map_host_physical(physPml4));
    pml4e_64 pml4e = pml4[host_virtual_address.pml4_idx];

    if (pml4e.present == 0 || pml4e.page_frame_number == 0)
    {
        return 0;
    }

    auto physPdpt = (pml4e.page_frame_number << 12);
    pdpte_64 *pdpt = reinterpret_cast<pdpte_64 *>(map_host_physical(physPdpt));
    pdpte_64 pdpte = pdpt[host_virtual_address.pdpt_idx];

    if (pdpte.present == 0 || pdpte.page_frame_number == 0)
    {
        return 0;
    }

    if (pdpte.large_page == 1)
    {
        pdpte_1gb_64 large_pdpte = {.flags = pdpte.flags};
        const std::uint64_t page_offset = host_virtual_address.offset_1gb;

        if (size_left_of_page != nullptr)
        {
            *size_left_of_page = (1ull << 30) - page_offset;
        }

        return (large_pdpte.page_frame_number << 30) + page_offset;
    }

    auto physPd = (pdpte.page_frame_number << 12);
    pde_64 *pd = reinterpret_cast<pde_64 *>(map_host_physical(physPd));
    pde_64 pde = pd[host_virtual_address.pd_idx];

    if (pde.present == 0 || pde.page_frame_number == 0)
    {
        return 0;
    }

    if (pde.large_page == 1)
    {
        pde_2mb_64 large_pde = {.flags = pde.flags};
        const std::uint64_t page_offset = host_virtual_address.offset_2mb;

        if (size_left_of_page != nullptr)
        {
            *size_left_of_page = (1ull << 21) - page_offset;
        }

        return (large_pde.page_frame_number << 21) + page_offset;
    }

    auto physPt = (pde.page_frame_number << 12);
    pte_64 *pt = reinterpret_cast<pte_64 *>(map_host_physical(physPt));
    pte_64 pte = pt[host_virtual_address.pt_idx];

    if (pte.present == 0 || pte.page_frame_number == 0)
    {
        return 0;
    }

    const std::uint64_t page_offset = host_virtual_address.offset_4kb;

    if (size_left_of_page != nullptr)
    {
        *size_left_of_page = (1ull << 12) - page_offset;
    }

    return (pte.page_frame_number << 12) + page_offset;
}

std::uint64_t memory_manager::operate_on_guest_virtual_memory(cr3 slat_cr3, void *host_buffer,
                                                              std::uint64_t guest_virtual_address, cr3 guest_cr3,
                                                              std::uint64_t total_size, memory_operation_t operation)
{
    std::uint64_t size_left_to_read = total_size;

    std::uint64_t bytes_read = 0;

    while (size_left_to_read != 0)
    {
        std::uint64_t size_left_of_virtual_page = 0;
        std::uint64_t size_left_of_slat_page = 0;

        std::uint64_t guest_physical_address = translate_guest_virtual_address(
            guest_cr3, slat_cr3, virtual_address_t{.value = guest_virtual_address + bytes_read},
            &size_left_of_virtual_page);

        if (guest_physical_address == 0)
        {
            break;
        }

        void *guest_physical_mapped =
            reinterpret_cast<void *>(map_guest_physical(slat_cr3, guest_physical_address, &size_left_of_slat_page));
        std::uint8_t *current_host_buffer = static_cast<std::uint8_t *>(host_buffer) + bytes_read;

        std::uint64_t size_left_of_pages = crt::min(size_left_of_virtual_page, size_left_of_slat_page);

        std::uint64_t copy_size = crt::min(size_left_to_read, size_left_of_pages);

        if (operation == memory_operation_t::write_operation)
        {
            crt::copy_memory(guest_physical_mapped, current_host_buffer, copy_size);
        }
        else
        {
            crt::copy_memory(current_host_buffer, guest_physical_mapped, copy_size);
        }

        size_left_to_read -= copy_size;
        bytes_read += copy_size;
    }

    return bytes_read;
}
