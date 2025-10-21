#include <Windows.h>
#include <fstream>
#include <cstdio>
#include "..\..\hyperv-core\include\hyperv\core.h"
#include "..\..\hyperv-core\include\hyperv\memory.h"

static void *allocate_locked_memory(const std::uint64_t size)
{
    void *const buffer = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (!buffer || !VirtualLock(buffer, size))
    {
        return nullptr;
    }

    return buffer;
}

int main()
{
    if (!hyperv::initialize())
    {
        printf("failed to initialize hyper-rev core!\n");
        return EXIT_FAILURE;
    }

    _wmkdir(L"physical_memory_dump");

    size_t read_size = 4096;
    auto buffer = allocate_locked_memory(read_size);
    uint64_t physical_address = 0;
    size_t max_read_size = 256 * 1024 * 1024; // 256 MB

    printf("attempting to read %llu MB of physical memory...\n", max_read_size / (1024 * 1024));

    for (physical_address = 0; physical_address < max_read_size; physical_address += read_size)
    {
        RtlZeroMemory(buffer, read_size);
        const uint64_t bytes_read = hyperv::memory::read_physical(buffer, physical_address, read_size);
        if (bytes_read == read_size)
        {
            char fileName[MAX_PATH];
            sprintf_s(fileName, "physical_memory_dump\\dump_0x%llX.bin", physical_address);

            std::ofstream f(fileName, std::ios::binary);
            f.write(reinterpret_cast<const char *>(buffer), read_size);
            f.close();
        }
        else
        {
            printf("failed to read physical address 0x%llX\n", physical_address);
        }
    }

    printf("finished reading physical memory.\n");
    hyperv::shutdown();
    return EXIT_SUCCESS;
}