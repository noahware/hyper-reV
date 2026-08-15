#pragma once
#include <span>
#include <unordered_map>
#include <string>
#include "system_def.h"

constexpr std::size_t page_size = 0x1000;

namespace sys
{
	struct kernel_module_t;

	std::uint8_t set_up();
	void clean_up();

	namespace kernel
	{
		std::uint8_t parse_modules();
		std::uint8_t dump_module_to_disk(std::string_view target_module_name, std::string_view output_directory);

		std::unordered_map<std::string, std::uint64_t> compile_symbol_list();

		inline std::unordered_map<std::string, kernel_module_t> modules_list = {};
	}

	namespace user
	{
		std::uint32_t query_system_information(std::int32_t information_class, void* information_out,
		                                       std::uint32_t information_size, std::uint32_t* returned_size);

		std::uint32_t adjust_privilege(std::uint32_t privilege, std::uint8_t enable, std::uint8_t current_thread_only,
		                               std::uint8_t* previous_enabled_state);
		std::uint8_t set_debug_privilege(std::uint8_t state, std::uint8_t* previous_state);

		void* allocate_locked_memory(std::uint64_t size, std::uint32_t protection);
		std::uint8_t free_memory(void* address);

		std::string to_string(std::wstring_view wstring);
	}

	namespace fs
	{
		std::uint8_t exists(std::string_view path);

		std::vector<std::uint8_t> read_from_disk(const std::string& full_path);
		std::uint8_t write_to_disk(const std::string& full_path, std::span<const std::uint8_t> buffer);
	}

	struct kernel_module_t
	{
		std::unordered_map<std::string, std::uint64_t> exports;

		std::uint64_t base_address{};
		std::uint32_t size{};
	};

	inline std::uint64_t current_cr3 = 0;
}
