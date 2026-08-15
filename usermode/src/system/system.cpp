#include "system.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "../hypercall/hypercall.h"
#include "../hook/hook.h"
#include "../logs/logs.h"

#include <portable_executable/image.hpp>

#include <vector>
#include <Windows.h>
#include <winternl.h>
#include <intrin.h>

extern "C" NTSTATUS NTAPI RtlAdjustPrivilege(std::uint32_t privilege, std::uint8_t enable, std::uint8_t current_thread,
                                             std::uint8_t* previous_enabled_state);

static std::vector<std::uint8_t> dump_kernel_module(const std::uint64_t module_base_address)
{
	constexpr std::uint64_t headers_size = page_size;

	std::vector<std::uint8_t> headers(headers_size);

	const std::uint64_t bytes_read = hypercall::read_guest_virtual_memory(
		headers.data(), module_base_address, sys::current_cr3, headers_size);

	if (bytes_read != headers_size)
	{
		return {};
	}

	const std::uint16_t magic = *reinterpret_cast<std::uint16_t*>(headers.data());

	if (magic != 0x5a4d)
	{
		return {};
	}

	const portable_executable::image_t* image = reinterpret_cast<portable_executable::image_t*>(headers.data());

	std::vector<std::uint8_t> image_buffer(image->nt_headers()->optional_header.size_of_image);

	memcpy(image_buffer.data(), headers.data(), page_size);

	for (const auto& current_section : image->sections())
	{
		const std::uint64_t read_offset = current_section.virtual_address;
		const std::uint64_t read_size = current_section.virtual_size;

		hypercall::read_guest_virtual_memory(image_buffer.data() + read_offset, module_base_address + read_offset,
		                                     sys::current_cr3, read_size);
	}

	return image_buffer;
}

static std::uint64_t find_kernel_detour_holder_base_address(const portable_executable::image_t* const ntoskrnl,
                                                            const std::uint64_t ntoskrnl_base_address)
{
	for (const auto& current_section : ntoskrnl->sections())
	{
		const std::string_view current_section_name(current_section.name);

		if (current_section.characteristics.mem_execute == 1 && current_section_name.contains("Pad"))
		{
			return ntoskrnl_base_address + current_section.virtual_address;
		}
	}

	return 0;
}

static std::unordered_map<std::string, std::uint64_t> parse_module_exports(
	const portable_executable::image_t* const image, const std::string& module_name,
	const std::uint64_t module_base_address)
{
	std::unordered_map<std::string, std::uint64_t> exports = {};

	for (const auto& current_export : image->exports())
	{
		const std::string current_export_name = module_name + "!" + current_export.name;
		const std::uint64_t delta = reinterpret_cast<std::uint64_t>(current_export.address) - image->as<
			std::uint64_t>();

		exports[current_export_name] = module_base_address + delta;
	}

	return exports;
}

static void add_module_to_list(const std::string& module_name, const std::span<const std::uint8_t> module_dump,
                               const std::uint64_t module_base_address, const std::uint32_t module_size)
{
	sys::kernel_module_t kernel_module = {};

	const auto* const image = reinterpret_cast<const portable_executable::image_t*>(module_dump.data());

	kernel_module.exports = parse_module_exports(image, module_name, module_base_address);
	kernel_module.base_address = module_base_address;
	kernel_module.size = module_size;

	sys::kernel::modules_list[module_name] = kernel_module;
}

static void erase_unused_modules(const std::unordered_map<std::string, sys::kernel_module_t>& modules_not_found)
{
	for (const auto& [module_name, module_info] : modules_not_found)
	{
		sys::kernel::modules_list.erase(module_name);
	}
}

// requires SeDebugPriviledge, use PsLoadedModulesList instead unless if using before ntoskrnl.exe is parsed
static std::vector<rtl_process_module_information_t> get_loaded_modules_priviledged()
{
	std::uint32_t size_of_information = 0;

	sys::user::query_system_information(11, nullptr, 0, &size_of_information);

	if (size_of_information == 0)
	{
		return {};
	}

	std::vector<std::uint8_t> buffer(size_of_information);

	const std::uint32_t status = sys::user::query_system_information(11, buffer.data(), size_of_information,
	                                                                 &size_of_information);

	if (NT_SUCCESS(status) == false)
	{
		return {};
	}

	auto* process_modules = reinterpret_cast<rtl_process_modules_t*>(buffer.data());

	rtl_process_module_information_t* start = &process_modules->modules[0];
	rtl_process_module_information_t* end = start + process_modules->module_count;

	return {start, end};
}

template <class t>
static t read_kernel_virtual_memory(const std::uint64_t address)
{
	t buffer = t();

	hypercall::read_guest_virtual_memory(&buffer, address, sys::current_cr3, sizeof(t));

	return buffer;
}

static std::wstring read_unicode_string(const std::uint64_t address)
{
	const auto length = read_kernel_virtual_memory<std::uint16_t>(address);

	if (length == 0)
	{
		return {};
	}

	const auto buffer_address = read_kernel_virtual_memory<std::uint64_t>(address + 8);

	std::wstring string(length / 2, L'\0');

	hypercall::read_guest_virtual_memory(string.data(), buffer_address, sys::current_cr3, length);

	return string;
}

static std::uint64_t get_ps_loaded_module_list()
{
	const std::string ntoskrnl_name = "ntoskrnl.exe";

	if (static_cast<int>(sys::kernel::modules_list.contains(ntoskrnl_name)) == 0)
	{
		return 0;
	}

	sys::kernel_module_t& ntoskrnl = sys::kernel::modules_list[ntoskrnl_name];

	const std::string ps_loaded_module_list_name = "ntoskrnl.exe!PsLoadedModuleList";

	return ntoskrnl.exports[ps_loaded_module_list_name];
}

std::uint8_t sys::kernel::parse_modules()
{
	const std::uint64_t ps_loaded_module_list = get_ps_loaded_module_list();

	if (ps_loaded_module_list == 0)
	{
		LOG_ERR("can't locate PsLoadedModuleList");

		return 0;
	}

	std::unordered_map<std::string, kernel_module_t> modules_not_found = modules_list;

	const std::uint64_t start_entry = ps_loaded_module_list;

	auto current_entry = read_kernel_virtual_memory<std::uint64_t>(start_entry); // flink

	while (current_entry != start_entry)
	{
		const kernel_module_t kernel_module = {};

		const auto module_base_address = read_kernel_virtual_memory<std::uint64_t>(current_entry + 0x30); // DllBase
		const auto module_size = read_kernel_virtual_memory<std::uint32_t>(current_entry + 0x40); // SizeOfImage
		const std::string module_name = user::to_string(read_unicode_string(current_entry + 0x58)); // BaseDllName

		// current_entry must not be accessed after this point in this iteration
		current_entry = read_kernel_virtual_memory<std::uint64_t>(current_entry); // flink

		if (modules_list.contains(module_name))
		{
			modules_not_found.erase(module_name);

			const kernel_module_t already_present_module = modules_list[module_name];

			if (already_present_module.base_address == module_base_address && already_present_module.size ==
				module_size)
			{
				continue;
			}
		}

		const std::vector<std::uint8_t> module_dump = dump_kernel_module(module_base_address);

		if (module_dump.empty())
		{
			continue;
		}

		add_module_to_list(module_name, module_dump, module_base_address, module_size);
	}

	erase_unused_modules(modules_not_found);

	return 1;
}

static void fix_dump(std::vector<std::uint8_t>& buffer)
{
	auto* image = reinterpret_cast<portable_executable::image_t*>(buffer.data());

	for (auto& current_section : image->sections())
	{
		current_section.pointer_to_raw_data = current_section.virtual_address;
		current_section.size_of_raw_data = current_section.virtual_size;
	}
}

std::uint8_t sys::kernel::dump_module_to_disk(const std::string_view target_module_name,
                                              const std::string_view output_directory)
{
	const auto module_info = modules_list[target_module_name.data()];

	const std::uint64_t module_base_address = module_info.base_address;

	if (module_base_address == 0)
	{
		return 0;
	}

	std::vector<std::uint8_t> buffer = dump_kernel_module(module_base_address);

	if (static_cast<int>(buffer.empty()) == 1)
	{
		return 0;
	}

	fix_dump(buffer);

	const std::string output_path = std::string(output_directory) + "\\" + "dump_" + std::string(target_module_name);

	return fs::write_to_disk(output_path, buffer);
}

std::unordered_map<std::string, std::uint64_t> sys::kernel::compile_symbol_list()
{
	std::unordered_map<std::string, std::uint64_t> aliases = { {"current_cr3", current_cr3} };

	for (auto& [module_name, module_info] : modules_list)
	{
		aliases.insert({ module_name, module_info.base_address });
		aliases.insert(module_info.exports.begin(), module_info.exports.end());
	}

	return aliases;
}

struct ntoskrnl_information_t
{
	std::uint64_t base_address;
	std::uint32_t size;

	std::vector<std::uint8_t> dump;
};

static std::optional<ntoskrnl_information_t> load_ntoskrnl_information()
{
	std::uint8_t desired_privilege_state = 1;
	std::uint8_t previous_privilege_state = 0;

	if (sys::user::set_debug_privilege(desired_privilege_state, &previous_privilege_state) == 0)
	{
		LOG_ERR("unable to acquire necessary privilege");

		return std::nullopt;
	}

	const std::vector<rtl_process_module_information_t> loaded_modules = get_loaded_modules_priviledged();

	sys::user::set_debug_privilege(previous_privilege_state, &desired_privilege_state);

	for (const rtl_process_module_information_t& current_module : loaded_modules)
	{
		const std::string_view current_module_name = reinterpret_cast<const char*>(current_module.full_path_name +
			current_module.offset_to_file_name);

		if (current_module_name == "ntoskrnl.exe")
		{
			const std::vector<std::uint8_t> ntoskrnl_dump = dump_kernel_module(current_module.image_base);

			if (ntoskrnl_dump.empty())
			{
				LOG_ERR("unable to dump ntoskrnl.exe");

				return std::nullopt;
			}

			ntoskrnl_information_t ntoskrnl_info = {};

			ntoskrnl_info.base_address = current_module.image_base;
			ntoskrnl_info.size = current_module.image_size;
			ntoskrnl_info.dump = ntoskrnl_dump;

			return ntoskrnl_info;
		}
	}

	return std::nullopt;
}

static std::uint8_t parse_ntoskrnl()
{
	const std::optional<ntoskrnl_information_t> ntoskrnl_info = load_ntoskrnl_information();

	if (!ntoskrnl_info)
	{
		LOG_ERR("unable to load ntoskrnl.exe's information");

		return 0;
	}

	const std::span<const std::uint8_t> ntoskrnl_dump = ntoskrnl_info->dump;
	const auto* const ntoskrnl_image = reinterpret_cast<const portable_executable::image_t*>(ntoskrnl_dump.data());

	add_module_to_list("ntoskrnl.exe", ntoskrnl_dump, ntoskrnl_info->base_address, ntoskrnl_info->size);

	const std::uint64_t kernel_detour_holder_base = find_kernel_detour_holder_base_address(
		ntoskrnl_image, ntoskrnl_info->base_address);

	if (kernel_detour_holder_base == 0)
	{
		LOG_ERR("unable to locate kernel hook holder");

		return 0;
	}

	hook::set_kernel_detour_holder_base(kernel_detour_holder_base);

	return 1;
}

std::uint8_t sys::set_up()
{
	current_cr3 = hypercall::read_guest_cr3();

	if (current_cr3 <= page_size)
	{
		LOG_ERR("hyperv-attachment doesn't seem to be loaded");

		return 0;
	}

	if (parse_ntoskrnl() == 0)
	{
		LOG_ERR("unable to parse ntoskrnl.exe");

		return 0;
	}

	if (kernel::parse_modules() == 0)
	{
		LOG_ERR("unable to parse kernel modules");

		return 0;
	}

	return 1;
}

void sys::clean_up()
{
	hook::clean_up();
}

std::uint32_t sys::user::query_system_information(std::int32_t information_class, void* information_out,
                                                  const std::uint32_t information_size, std::uint32_t* returned_size)
{
	return NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(information_class), information_out,
	                                information_size, reinterpret_cast<ULONG*>(returned_size));
}

std::uint32_t sys::user::adjust_privilege(const std::uint32_t privilege, const std::uint8_t enable,
                                          const std::uint8_t current_thread_only, std::uint8_t* previous_enabled_state)
{
	return RtlAdjustPrivilege(privilege, enable, current_thread_only, previous_enabled_state);
}

std::uint8_t sys::user::set_debug_privilege(const std::uint8_t state, std::uint8_t* previous_state)
{
	constexpr std::uint32_t debug_privilege_id = 20;

	const std::uint32_t status = adjust_privilege(debug_privilege_id, state, 0, previous_state);

	return NT_SUCCESS(status);
}

void* sys::user::allocate_locked_memory(const std::uint64_t size, const std::uint32_t protection)
{
	void* allocation_base = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, protection);

	if (allocation_base == nullptr)
	{
		return nullptr;
	}

	const std::int32_t lock_status = VirtualLock(allocation_base, size);

	if (lock_status == 0)
	{
		free_memory(allocation_base);

		return nullptr;
	}

	return allocation_base;
}

std::uint8_t sys::user::free_memory(void* address)
{
	const std::int32_t free_status = VirtualFree(address, 0, MEM_RELEASE);

	return static_cast<std::uint8_t>(free_status != 0);
}

std::string sys::user::to_string(const std::wstring_view wstring)
{
	if (static_cast<int>(wstring.empty()) == 1)
	{
		return {};
	}

	std::string converted_string = {};

	std::ranges::transform(wstring,
	                       std::back_inserter(converted_string), [](const wchar_t character)
	                       {
		                       return static_cast<char>(character);
	                       });

	return converted_string;
}

std::uint8_t sys::fs::exists(const std::string_view path)
{
	return static_cast<std::uint8_t>(std::filesystem::exists(path));
}

std::vector<std::uint8_t> sys::fs::read_from_disk(const std::string& full_path)
{
	std::ifstream file(full_path, std::ios::binary);

	if (file.is_open())
	{
		return { std::istreambuf_iterator(file), { } };
	}

	return { };
}

std::uint8_t sys::fs::write_to_disk(const std::string& full_path, const std::span<const std::uint8_t> buffer)
{
	std::ofstream file(full_path.data(), std::ios::binary);

	if (static_cast<int>(file.is_open()) == 0)
	{
		return 0;
	}

	file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

	return static_cast<std::uint8_t>(file.good());
}
