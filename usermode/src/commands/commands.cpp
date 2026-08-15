#include "commands.h"
#include <CLI/CLI.hpp>
#include <hypercall/hypercall_def.h>
#include "../hook/hook.h"
#include "../hypercall/hypercall.h"
#include "../system/system.h"
#include "../logs/logs.h"

#include <array>
#include <ranges>

#include "../script/script.h"

#define INVOKE_COMMAND_PROCESSOR(command) process_##command(command)
#define PROCESS_INITIAL_COMMAND(command) if (*command) INVOKE_COMMAND_PROCESSOR(command)
#define PROCESS_COMMAND(command) else if (*command) INVOKE_COMMAND_PROCESSOR(command)

template <class T>
static T get_command_option(const CLI::App* const app, const std::string& option_name)
{
	const auto* const option = app->get_option(option_name);

	return !option->empty() ? option->as<T>() : T{};
}

static CLI::Option* add_command_option(CLI::App* const app, const std::string& option_name)
{
	return app->add_option(option_name);
}

static CLI::Option* add_transformed_command_option(CLI::App* const app, const std::string& option_name,
                                                   const CLI::Transformer& transformer)
{
	auto* const option = add_command_option(app, option_name);

	return option->transform(transformer);
}

static std::uint8_t get_command_flag(const CLI::App* const app, const std::string& flag_name)
{
	const auto* const option = app->get_option(flag_name);

	return static_cast<std::uint8_t>(!option->empty());
}

static CLI::Option* add_command_flag(CLI::App* app, const std::string& flag_name)
{
	return app->add_flag(flag_name);
}

static CLI::App* init_rgpm(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* rgpm = app.add_subcommand("rgpm", "reads memory from a given guest physical address")->ignore_case();

	add_transformed_command_option(rgpm, "physical_address", aliases_transformer)->required();
	add_command_option(rgpm, "size")->check(CLI::Range(0, 8))->required();

	return rgpm;
}

static void process_rgpm(const CLI::App* const rgpm)
{
	const auto guest_physical_address = get_command_option<std::uint64_t>(rgpm, "physical_address");
	const auto size = get_command_option<std::uint64_t>(rgpm, "size");

	std::uint64_t value = 0;

	const std::uint64_t bytes_read = hypercall::read_guest_physical_memory(&value, guest_physical_address, size);

	if (bytes_read == size)
	{
		LOG_INFO("value: 0x{:x}", value);
	}
	else
	{
		LOG_ERR("failed to read");
	}
}

static CLI::App* init_wgpm(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* wgpm = app.add_subcommand("wgpm", "writes memory to a given guest physical address")->ignore_case();

	add_transformed_command_option(wgpm, "physical_address", aliases_transformer)->required();
	add_command_option(wgpm, "value")->required();
	add_command_option(wgpm, "size")->check(CLI::Range(0, 8))->required();

	return wgpm;
}

static void process_wgpm(const CLI::App* const wgpm)
{
	const auto guest_physical_address = get_command_option<std::uint64_t>(wgpm, "physical_address");
	const auto size = get_command_option<std::uint64_t>(wgpm, "size");

	const auto value = get_command_option<std::uint64_t>(wgpm, "value");

	const std::uint64_t bytes_written = hypercall::write_guest_physical_memory(&value, guest_physical_address, size);

	if (bytes_written == size)
	{
		LOG_INFO("success in write");
	}
	else
	{
		LOG_ERR("failed to write");
	}
}

static CLI::App* init_cgpm(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* cgpm = app.add_subcommand(
		"cgpm", "copies memory from a given source to a destination (guest physical addresses)")->ignore_case();

	add_transformed_command_option(cgpm, "destination_physical_address", aliases_transformer)->required();
	add_transformed_command_option(cgpm, "source_physical_address", aliases_transformer)->required();
	add_command_option(cgpm, "size")->required();

	return cgpm;
}

static void process_cgpm(const CLI::App* const cgpm)
{
	const auto guest_destination_physical_address = get_command_option<std::uint64_t>(
		cgpm, "destination_physical_address");
	const auto guest_source_physical_address = get_command_option<std::uint64_t>(cgpm, "source_physical_address");
	const auto size = get_command_option<std::uint64_t>(cgpm, "size");

	std::vector<std::uint8_t> buffer(size);

	const std::uint64_t bytes_read = hypercall::read_guest_physical_memory(
		buffer.data(), guest_source_physical_address, size);
	const std::uint64_t bytes_written = hypercall::write_guest_physical_memory(
		buffer.data(), guest_destination_physical_address, size);

	if (bytes_read == size && bytes_written == size)
	{
		LOG_INFO("success in copy");
	}
	else
	{
		LOG_ERR("failed to copy");
	}
}

static CLI::App* init_gvat(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* gvat = app.add_subcommand(
		                    "gvat",
		                    "translates a guest virtual address to its corresponding guest physical address, with the given guest cr3 value")
	                    ->ignore_case();

	add_transformed_command_option(gvat, "virtual_address", aliases_transformer)->required();
	add_transformed_command_option(gvat, "cr3", aliases_transformer)->required();

	return gvat;
}

static void process_gvat(const CLI::App* const gvat)
{
	const auto virtual_address = get_command_option<std::uint64_t>(gvat, "virtual_address");
	const auto cr3 = get_command_option<std::uint64_t>(gvat, "cr3");

	const std::uint64_t physical_address = hypercall::translate_guest_virtual_address(virtual_address, cr3);

	LOG_INFO("physical address: 0x{:x}", physical_address);
}

static CLI::App* init_rgvm(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* rgvm = app.add_subcommand(
		                    "rgvm",
		                    "reads memory from a given guest virtual address (when given the corresponding guest cr3 value)")
	                    ->
	                    ignore_case();

	add_transformed_command_option(rgvm, "virtual_address", aliases_transformer)->required();
	add_transformed_command_option(rgvm, "cr3", aliases_transformer)->required();
	add_command_option(rgvm, "size")->check(CLI::Range(0, 8))->required();

	return rgvm;
}

static void process_rgvm(const CLI::App* const rgvm)
{
	const auto guest_virtual_address = get_command_option<std::uint64_t>(rgvm, "virtual_address");
	const auto cr3 = get_command_option<std::uint64_t>(rgvm, "cr3");
	const auto size = get_command_option<std::uint64_t>(rgvm, "size");

	std::uint64_t value = 0;

	const std::uint64_t bytes_read = hypercall::read_guest_virtual_memory(&value, guest_virtual_address, cr3, size);

	if (bytes_read == size)
	{
		LOG_INFO("value: 0x{:x}", value);
	}
	else
	{
		LOG_ERR("failed to read");
	}
}

static CLI::App* init_wgvm(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* wgvm = app.add_subcommand(
		                    "wgvm",
		                    "writes memory from a given guest virtual address (when given the corresponding guest cr3 value)")
	                    ->
	                    ignore_case();

	add_transformed_command_option(wgvm, "virtual_address", aliases_transformer)->required();
	add_transformed_command_option(wgvm, "cr3", aliases_transformer)->required();
	add_command_option(wgvm, "value")->required();
	add_command_option(wgvm, "size")->check(CLI::Range(0, 8))->required();

	return wgvm;
}

static void process_wgvm(const CLI::App* const wgvm)
{
	const auto guest_virtual_address = get_command_option<std::uint64_t>(wgvm, "virtual_address");
	const auto cr3 = get_command_option<std::uint64_t>(wgvm, "cr3");
	const auto size = get_command_option<std::uint64_t>(wgvm, "size");

	const auto value = get_command_option<std::uint64_t>(wgvm, "value");

	const std::uint64_t bytes_written = hypercall::write_guest_virtual_memory(&value, guest_virtual_address, cr3, size);

	if (bytes_written == size)
	{
		LOG_INFO("success in write at given address");
	}
	else
	{
		LOG_ERR("failed to write at given address");
	}
}

static CLI::App* init_cgvm(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* cgvm = app.add_subcommand(
		                    "cgvm",
		                    "copies memory from a given source to a destination (guest virtual addresses) (when given the corresponding guest cr3 values)")
	                    ->ignore_case();

	add_transformed_command_option(cgvm, "destination_virtual_address", aliases_transformer)->required();
	add_transformed_command_option(cgvm, "destination_cr3", aliases_transformer)->required();
	add_transformed_command_option(cgvm, "source_virtual_address", aliases_transformer)->required();
	add_transformed_command_option(cgvm, "source_cr3", aliases_transformer)->required();
	add_command_option(cgvm, "size")->required();

	return cgvm;
}

static void process_cgvm(const CLI::App* const wgvm)
{
	const auto guest_destination_virtual_address = get_command_option<std::uint64_t>(
		wgvm, "destination_virtual_address");
	const auto guest_destination_cr3 = get_command_option<std::uint64_t>(wgvm, "destination_cr3");

	const auto guest_source_virtual_address = get_command_option<std::uint64_t>(wgvm, "source_virtual_address");
	const auto guest_source_cr3 = get_command_option<std::uint64_t>(wgvm, "source_cr3");

	const auto size = get_command_option<std::uint64_t>(wgvm, "size");

	std::vector<std::uint8_t> buffer(size);

	const std::uint64_t bytes_read = hypercall::read_guest_virtual_memory(
		buffer.data(), guest_source_virtual_address, guest_source_cr3, size);
	const std::uint64_t bytes_written = hypercall::write_guest_virtual_memory(
		buffer.data(), guest_destination_virtual_address, guest_destination_cr3, size);

	if (bytes_read == size && bytes_written == size)
	{
		LOG_INFO("success in copy");
	}
	else
	{
		LOG_ERR("failed to copy");
	}
}

static CLI::App* init_akh(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* const akh = app.add_subcommand(
		                         "akh",
		                         "add a hook on specified kernel code (given the guest virtual address) (asmbytes in form: 0xE8 0x12 0x23 0x34 0x45)")
	                         ->ignore_case();

	add_transformed_command_option(akh, "virtual_address", aliases_transformer)->required();
	add_command_option(akh, "--script_text")->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)->expected(-1);
	add_command_option(akh, "--script_file")->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)->expected(-1);
	add_command_option(akh, "--asmbytes")->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)->expected(-1);
	add_command_option(akh, "--post_original_asmbytes")->multi_option_policy(CLI::MultiOptionPolicy::TakeAll)->
	                                                     expected(-1);
	add_command_flag(akh, "--monitor");

	return akh;
}

static void process_akh(const CLI::App* const akh)
{
	const auto virtual_address = get_command_option<std::uint64_t>(akh, "virtual_address");

	auto asm_bytes = get_command_option<std::vector<uint8_t>>(akh, "--asmbytes");

	const auto script_contents = get_command_option<std::string>(akh, "--script_text");
	const auto script_file = get_command_option<std::string>(akh, "--script_file");

	if (!script_contents.empty())
	{
		const auto compiled_script_bytes = script::compile(script_contents);

		asm_bytes.insert_range(asm_bytes.end(), compiled_script_bytes);
	}
	else if (!script_file.empty())
	{
		const auto compiled_script_bytes = script::compile_from_path(script_file);

		asm_bytes.insert_range(asm_bytes.end(), compiled_script_bytes);
	}

	const auto post_original_asm_bytes = get_command_option<std::vector<uint8_t>>(akh, "--post_original_asmbytes");

	const std::uint8_t monitor = get_command_flag(akh, "--monitor");

	if (monitor == 1)
	{
		std::array<std::uint8_t, 9> monitor_bytes = {
			0x51, // push rcx
			0xB9, 0x00, 0x00, 0x00, 0x00, // mov ecx, 0
			0x0F, 0xA2, // cpuid
			0x59 // pop rcx
		};

		hypercall_info_t call_info = {};

		call_info.primary_key = hypercall_primary_key;
		call_info.secondary_key = hypercall_secondary_key;
		call_info.call_type = hypercall_type_t::log_current_state;

		*reinterpret_cast<std::uint32_t*>(&monitor_bytes[2]) = static_cast<std::uint32_t>(call_info.value);

		asm_bytes.insert(asm_bytes.end(), monitor_bytes.begin(), monitor_bytes.end());
	}

	const std::uint8_t hook_status = hook::add_kernel_hook(virtual_address, asm_bytes, post_original_asm_bytes);

	if (hook_status == 1)
	{
		LOG_INFO("success in hook");
	}
	else
	{
		LOG_ERR("failed to hook");
	}
}

static CLI::App* init_rkh(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* rkh = app.add_subcommand(
		                   "rkh",
		                   "remove a previously placed hook on specified kernel code (given the guest virtual address)")
	                   ->
	                   ignore_case();

	add_transformed_command_option(rkh, "virtual_address", aliases_transformer)->required();

	return rkh;
}

static void process_rkh(const CLI::App* const rkh)
{
	const auto virtual_address = get_command_option<std::uint64_t>(rkh, "virtual_address");

	const std::uint8_t hook_removal_status = hook::remove_kernel_hook(virtual_address, 1);

	if (hook_removal_status == 1)
	{
		LOG_INFO("success in hook removal");
	}
	else
	{
		LOG_ERR("failed to remove hook");
	}
}

static CLI::App* init_hgpp(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* hgpp = app.add_subcommand("hgpp", "hide a physical page's real contents from the guest")->ignore_case();

	add_transformed_command_option(hgpp, "physical_address", aliases_transformer)->required();

	return hgpp;
}

static void process_hgpp(const CLI::App* const hgpp)
{
	const auto physical_address = get_command_option<std::uint64_t>(hgpp, "physical_address");

	const std::uint64_t hide_status = hypercall::hide_guest_physical_page(physical_address);

	if (hide_status == 1)
	{
		LOG_INFO("success in hiding page");
	}
	else
	{
		LOG_ERR("failed to hide page");
	}
}

static CLI::App* init_fl(CLI::App& app)
{
	CLI::App* fl = app.add_subcommand("fl", "flush trap frame logs from hooks")->ignore_case();

	return fl;
}

static void process_fl([[maybe_unused]] const CLI::App* const fl)
{
	constexpr std::uint64_t log_count = 2840;
	constexpr std::uint64_t failed_log_count = -1;

	std::vector<trap_frame_log_t> logs(log_count);

	const std::uint64_t logs_flushed = hypercall::flush_logs(logs);

	if (logs_flushed == failed_log_count)
	{
		LOG_ERR("failed to flush logs");
	}
	else if (logs_flushed == 0)
	{
		LOG_INFO("there are no logs to flush");
	}
	else
	{
		LOG_INFO("success in flushing logs ({}), outputting logs now:\n\n", logs_flushed);

		for (std::uint64_t i = 0; i < logs_flushed; i++)
		{
			const trap_frame_log_t& log = logs[i];

			if (log.rip == 0)
			{
				break;
			}

			LOG_INFO(
				"{}. rip=0x{:X} rax=0x{:X} rcx=0x{:X}\nrdx=0x{:X} rbx=0x{:X} rsp=0x{:X} rbp=0x{:X}\nrsi=0x{:X} rdi=0x{:X} r8=0x{:X} r9=0x{:X}\nr10=0x{:X} r11=0x{:X} r12=0x{:X} r13=0x{:X} r14=0x{:X}\nr15=0x{:X} cr3=0x{:X}\n"
				, i, log.rip, log.rax, log.rcx, log.rdx, log.rbx, log.rsp, log.rbp, log.rsi, log.rdi, log.r8, log.r9,
				log.r10, log.r11, log.r12, log.r13, log.r14, log.r15, log.cr3);

			LOG_INFO("stack data:");

			for (const std::uint64_t stack_value : log.stack_data)
			{
				LOG_INFO("  0x{:X}", stack_value);
			}
		}
	}
}

static CLI::App* init_hfpc(CLI::App& app)
{
	CLI::App* hfpc = app.add_subcommand("hfpc", "get hyperv-attachment's heap free page count")->ignore_case();

	return hfpc;
}

static void process_hfpc([[maybe_unused]] CLI::App* const hfpc)
{
	const std::uint64_t heap_free_page_count = hypercall::get_heap_free_page_count();

	LOG_INFO("heap free page count: {}", heap_free_page_count);
}

static CLI::App* init_lkm(CLI::App& app)
{
	CLI::App* lkm = app.add_subcommand("lkm", "print list of loaded kernel modules")->ignore_case();

	return lkm;
}

static void process_lkm([[maybe_unused]] CLI::App* const lkm)
{
	for (const auto& [module_name, module_info] : sys::kernel::modules_list)
	{
		LOG_INFO("'{}' has a base address of: 0x{:x}, and a size of: 0x{:X}", module_name, module_info.base_address,
		             module_info.size);
	}
}

static CLI::App* init_kme(CLI::App& app)
{
	CLI::App* kme = app.add_subcommand("kme", "list the exports of a loaded kernel module (when given the name)")->
	                    ignore_case();

	add_command_option(kme, "module_name")->required();

	return kme;
}

static void process_kme(const CLI::App* const kme)
{
	const auto module_name = get_command_option<std::string>(kme, "module_name");

	if (!sys::kernel::modules_list.contains(module_name))
	{
		LOG_ERR("module not found");

		return;
	}

	const sys::kernel_module_t module = sys::kernel::modules_list[module_name];

	for (const auto& [export_name, export_address] : module.exports)
	{
		LOG_INFO("{} = 0x{:X}", export_name, export_address);
	}
}

static CLI::App* init_dkm(CLI::App& app)
{
	CLI::App* dkm = app.add_subcommand("dkm", "dump kernel module to a file on disk")->ignore_case();

	add_command_option(dkm, "module_name")->required();
	add_command_option(dkm, "output_directory")->required();

	return dkm;
}

static void process_dkm(const CLI::App* const dkm)
{
	const auto module_name = get_command_option<std::string>(dkm, "module_name");

	if (!sys::kernel::modules_list.contains(module_name))
	{
		LOG_ERR("module not found");

		return;
	}

	const auto output_directory = get_command_option<std::string>(dkm, "output_directory");

	const std::uint8_t status = sys::kernel::dump_module_to_disk(module_name, output_directory);

	if (status == 1)
	{
		LOG_INFO("success in dumping module");
	}
	else
	{
		LOG_ERR("failed to dump module");
	}
}

static CLI::App* init_gva(CLI::App& app, const CLI::Transformer& aliases_transformer)
{
	CLI::App* gva = app.add_subcommand("gva", "get the numerical value of an alias")->ignore_case();

	add_transformed_command_option(gva, "alias_name", aliases_transformer)->required();

	return gva;
}

static void process_gva(const CLI::App* const gva)
{
	const auto alias_value = get_command_option<std::uint64_t>(gva, "alias_name");

	LOG_INFO("alias value: 0x{:X}", alias_value);
}

void commands::process(const std::string& command)
{
	if (command.empty())
	{
		return;
	}

	CLI::App app;
	app.require_subcommand();

	sys::kernel::parse_modules();

	const std::unordered_map<std::string, std::uint64_t> aliases = sys::kernel::compile_symbol_list();

	const auto aliases_transformer = CLI::Transformer(aliases, CLI::ignore_case);

	const CLI::App* const rgpm = init_rgpm(app, aliases_transformer);
	const CLI::App* const wgpm = init_wgpm(app, aliases_transformer);
	const CLI::App* const cgpm = init_cgpm(app, aliases_transformer);
	const CLI::App* const gvat = init_gvat(app, aliases_transformer);
	const CLI::App* const rgvm = init_rgvm(app, aliases_transformer);
	const CLI::App* const wgvm = init_wgvm(app, aliases_transformer);
	const CLI::App* const cgvm = init_cgvm(app, aliases_transformer);
	const CLI::App* const akh = init_akh(app, aliases_transformer);
	const CLI::App* const rkh = init_rkh(app, aliases_transformer);
	const CLI::App* const gva = init_gva(app, aliases_transformer);
	const CLI::App* const hgpp = init_hgpp(app, aliases_transformer);
	const CLI::App* const fl = init_fl(app);
	CLI::App* const hfpc = init_hfpc(app);
	CLI::App* const lkm = init_lkm(app);
	const CLI::App* const kme = init_kme(app);
	const CLI::App* const dkm = init_dkm(app);

	try
	{
		app.parse(command);

		PROCESS_INITIAL_COMMAND(rgpm);
		PROCESS_COMMAND(wgpm);
		PROCESS_COMMAND(cgpm);
		PROCESS_COMMAND(gvat);
		PROCESS_COMMAND(rgvm);
		PROCESS_COMMAND(wgvm);
		PROCESS_COMMAND(cgvm);
		PROCESS_COMMAND(akh);
		PROCESS_COMMAND(rkh);
		PROCESS_COMMAND(gva);
		PROCESS_COMMAND(hgpp);
		PROCESS_COMMAND(fl);
		PROCESS_COMMAND(hfpc);
		PROCESS_COMMAND(lkm);
		PROCESS_COMMAND(kme);
		PROCESS_COMMAND(dkm);
	}
	catch (const CLI::ParseError& error)
	{
		app.exit(error);
	}
}
