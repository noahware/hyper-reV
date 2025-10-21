#include "..\pch.h"
#include <hypercall/hypercall_def.h>
#include <structures/memory_operation.h>
#include <cstdio>
#include <windows.h>

namespace hyperv::memory
{
constexpr std::uint64_t usermode_address_limit = 0x00007FFFFFFFFFFFULL;
constexpr std::uint64_t max_physical_address = 0x0000FFFFFFFFFFFFULL;

uint64_t read_physical(void *destination, std::uint64_t physical_address, std::uint64_t size)
{
    if (!destination || physical_address > max_physical_address || size == 0)
    {
        logger::log_error("read_physical: wrong parameters!");
        return 0;
    }

    std::uint64_t result = hypercall::read_guest_physical_memory(destination, physical_address, size);
    if (result != size)
    {
        if (is_translation_error(result))
        {
            logger::log_error("read_physical: destination=0x%p physical_address=0x%llX translation error: %d",
                              destination, physical_address, (uint8_t)get_translation_result(result));
            return UINT64_MAX;
        }
        else
        {
            logger::log_warning("read_physical: physical_address=0x%llX requested=0x%llX actual=0x%llX",
                                physical_address, size, result);
        }
    }
    return result;
}

uint64_t write_physical(const void *source, std::uint64_t physical_address, std::uint64_t size)
{
    logger::log_debug("write_physical - ENTRY: source=0x%p physical_address=0x%llX size=0x%llX", source,
                      physical_address, size);

    if (!source)
    {
        logger::log_error("write_physical - FAILED: source is NULL");
        return 0;
    }

    if (physical_address > max_physical_address)
    {
        logger::log_error("write_physical - FAILED: physical_address 0x%llX exceeds max 0x%llX", physical_address,
                          max_physical_address);
        return 0;
    }

    if (size == 0)
    {
        logger::log_warning("write_physical - WARNING: size is 0");
        return 0;
    }

    std::uint64_t result = hypercall::write_guest_physical_memory(const_cast<void *>(source), physical_address, size);

    if (result != size)
    {
        logger::log_warning("write_physical - PARTIAL: requested=0x%llX actual=0x%llX", size, result);
    }
    else
    {
        logger::log_debug("write_physical - SUCCESS: wrote 0x%llX bytes to 0x%llX", result, physical_address);
    }

    return result;
}

uint64_t read_host_physical(void *destination, std::uint64_t physical_address, std::uint64_t size)
{
    logger::log_debug("read_host_physical - ENTRY: destination=0x%p physical_address=0x%llX size=0x%llX", destination,
                      physical_address, size);

    if (!destination)
    {
        logger::log_error("read_host_physical - FAILED: destination is NULL");
        return 0;
    }

    if (physical_address > max_physical_address)
    {
        logger::log_error("read_host_physical - FAILED: physical_address 0x%llX exceeds max 0x%llX", physical_address,
                          max_physical_address);
        return 0;
    }

    if (size == 0)
    {
        logger::log_warning("read_host_physical - WARNING: size is 0");
        return 0;
    }

    std::uint64_t result = hypercall::read_host_physical_memory(destination, physical_address, size);

    if (result != size)
    {
        logger::log_warning("read_host_physical - PARTIAL: requested=0x%llX actual=0x%llX", size, result);
    }
    else
    {
        logger::log_debug("read_host_physical - SUCCESS: read 0x%llX bytes from host physical 0x%llX", result,
                          physical_address);
    }

    return result;
}

uint64_t write_host_physical(const void *source, std::uint64_t physical_address, std::uint64_t size)
{
    logger::log_debug("write_host_physical - ENTRY: source=0x%p physical_address=0x%llX size=0x%llX", source,
                      physical_address, size);

    if (!source)
    {
        logger::log_error("write_host_physical - FAILED: source is NULL");
        return 0;
    }

    if (physical_address > max_physical_address)
    {
        logger::log_error("write_host_physical - FAILED: physical_address 0x%llX exceeds max 0x%llX", physical_address,
                          max_physical_address);
        return 0;
    }

    if (size == 0)
    {
        logger::log_warning("write_host_physical - WARNING: size is 0");
        return 0;
    }

    std::uint64_t result = hypercall::write_host_physical_memory(const_cast<void *>(source), physical_address, size);

    if (result != size)
    {
        logger::log_warning("write_host_physical - PARTIAL: requested=0x%llX actual=0x%llX", size, result);
    }
    else
    {
        logger::log_debug("write_host_physical - SUCCESS: wrote 0x%llX bytes to host physical 0x%llX", result,
                          physical_address);
    }

    return result;
}

uint64_t read_virtual(void *destination, std::uint64_t virtual_address, std::uint64_t cr3, std::uint64_t size)
{
    logger::log_debug("read_virtual - ENTRY: destination=0x%p virtual_address=0x%llX cr3=0x%llX size=0x%llX",
                      destination, virtual_address, cr3, size);

    if (!destination)
    {
        logger::log_error("read_virtual - FAILED: destination is NULL");
        return 0;
    }

    if (virtual_address > usermode_address_limit && virtual_address < 0xFFFF800000000000ULL)
    {
        logger::log_warning("read_virtual - WARNING: virtual_address 0x%llX is in invalid range", virtual_address);
    }

    if (cr3 == 0)
    {
        logger::log_error("read_virtual - FAILED: cr3 is 0");
        return 0;
    }

    if (size == 0)
    {
        logger::log_warning("read_virtual - WARNING: size is 0");
        return 0;
    }

    std::uint64_t result = hypercall::read_guest_virtual_memory(destination, virtual_address, cr3, size);

    if (result != size)
    {
        logger::log_warning("read_virtual - PARTIAL: requested=0x%llX actual=0x%llX from VA 0x%llX (CR3=0x%llX)", size,
                            result, virtual_address, cr3);
    }
    else
    {
        logger::log_debug("read_virtual - SUCCESS: read 0x%llX bytes from VA 0x%llX (CR3=0x%llX)", result,
                          virtual_address, cr3);
    }

    return result;
}

uint64_t write_virtual(const void *source, std::uint64_t virtual_address, std::uint64_t cr3, std::uint64_t size)
{
    logger::log_debug("write_virtual - ENTRY: source=0x%p virtual_address=0x%llX cr3=0x%llX size=0x%llX", source,
                      virtual_address, cr3, size);

    if (!source)
    {
        logger::log_error("write_virtual - FAILED: source is NULL");
        return 0;
    }

    if (virtual_address > usermode_address_limit && virtual_address < 0xFFFF800000000000ULL)
    {
        logger::log_warning("write_virtual - WARNING: virtual_address 0x%llX is in invalid range", virtual_address);
    }

    if (cr3 == 0)
    {
        logger::log_error("write_virtual - FAILED: cr3 is 0");
        return 0;
    }

    if (size == 0)
    {
        logger::log_warning("write_virtual - WARNING: size is 0");
        return 0;
    }

    std::uint64_t result =
        hypercall::write_guest_virtual_memory(const_cast<void *>(source), virtual_address, cr3, size);

    if (result != size)
    {
        logger::log_warning("write_virtual - PARTIAL: requested=0x%llX actual=0x%llX to VA 0x%llX (CR3=0x%llX)", size,
                            result, virtual_address, cr3);
    }
    else
    {
        logger::log_debug("write_virtual - SUCCESS: wrote 0x%llX bytes to VA 0x%llX (CR3=0x%llX)", result,
                          virtual_address, cr3);
    }

    return result;
}

uint64_t virtual_to_physical(std::uint64_t virtual_address, std::uint64_t cr3)
{
    logger::log_debug("virtual_to_physical - ENTRY: virtual_address=0x%llX cr3=0x%llX", virtual_address, cr3);

    if (cr3 == 0)
    {
        logger::log_error("virtual_to_physical - FAILED: cr3 is 0");
        return 0;
    }

    if (virtual_address > usermode_address_limit && virtual_address < 0xFFFF800000000000ULL)
    {
        logger::log_warning("virtual_to_physical - WARNING: virtual_address 0x%llX is in invalid range",
                            virtual_address);
    }

    std::uint64_t result = hypercall::translate_guest_virtual_address(virtual_address, cr3);

    if (result == 0)
    {
        logger::log_error("virtual_to_physical - FAILED: translation failed for VA 0x%llX (CR3=0x%llX)",
                          virtual_address, cr3);
    }
    else
    {
        logger::log_debug("virtual_to_physical - SUCCESS: VA 0x%llX -> PA 0x%llX (CR3=0x%llX)", virtual_address, result,
                          cr3);
    }

    return result;
}

std::uint64_t get_current_cr3()
{
    logger::log_trace("get_current_cr3 - ENTRY");

    std::uint64_t result = hypercall::read_guest_cr3();

    if (result == 0)
    {
        logger::log_warning("get_current_cr3 - WARNING: returned CR3 is 0");
    }
    else
    {
        logger::log_trace("get_current_cr3 - SUCCESS: CR3=0x%llX", result);
    }

    return result;
}

} // namespace hyperv::memory