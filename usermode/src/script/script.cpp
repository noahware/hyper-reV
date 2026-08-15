#include "script.h"
#include "../assembler/assembler.h"
#include "../system/system.h"
#include "../logs/logs.h"

#include <regex>

static std::string normalise_assembly(const std::string_view input, const std::unordered_map<std::string, std::uint64_t>& symbols)
{
    std::string output(input.begin(), input.end());

    std::erase(output, '\r');

    output = std::regex_replace(output, std::regex(R"(\n\s*\n+)"), "\n");

    std::ranges::replace(output, '\n', ';');

    if (!output.ends_with(';'))
    {
        output.push_back(';');
    }

    const auto search_fn = [&output](const std::size_t offset) -> std::size_t
        {
            return output.find(';', offset);
        };

    std::size_t previous = 0;

    for (std::size_t i = search_fn(0); i != std::string::npos; i = search_fn(i + 1))
    {
        if (previous)
        {
            previous++;
        }

        while (previous < output.size() && output[previous] == ' ')
        {
            previous++;
        }

        std::size_t current = i;

        while (1 < current && output[current - 1] == ' ')
        {
            current--;
        }

        if (current < previous)
        {
            continue;
        }

        const std::size_t count = current - previous;

        const std::string_view instruction(output.data() + static_cast<std::uint32_t>(previous), count);

        if (instruction.starts_with("limp"))
        {
            const auto comma = instruction.find_first_of(',');

            if (comma != std::string::npos)
            {
                const auto symbol_start = instruction.find_first_not_of(' ', comma + 1);

                if (symbol_start != std::string::npos)
                {
                    const std::size_t symbol_count = instruction.size() - symbol_start;
                    const std::string symbol_view(instruction.data() + symbol_start, symbol_count);

                    LOG_INFO("found import to normalise at index {} with value '{}' {}, {}", i, symbol_view, comma, symbol_start);

                    if (!symbols.contains(symbol_view))
                    {
                        throw std::runtime_error(std::format("unable to find symbol '{}'", symbol_view));
                    }

                    const std::uint64_t symbol_address = symbols.at(symbol_view);

                    const std::string formatted_address = std::format("0x{:X}", symbol_address);

                    output.erase(previous + symbol_start, symbol_count);
                    output.insert(previous + symbol_start, formatted_address);
                    output.erase(previous, 4);
                    output.insert(previous, "mov");

                    i -= symbol_count - formatted_address.size() + 1;
                }
            }
        }

        previous = i;
    }

    return output;
}

std::vector<std::uint8_t> script::compile(const std::string_view script_contents)
{
	keystone_assembler assembler = keystone_assembler::create(KS_ARCH_X86, KS_MODE_64);

	assembler.set_syntax(KS_OPT_SYNTAX_INTEL);

	try
	{
        const auto symbols = sys::kernel::compile_symbol_list();
        const auto normalised_script = normalise_assembly(script_contents, symbols);

        const auto encoding = assembler.assemble(normalised_script);

        if (!encoding)
        {
            const auto error = encoding.error();

            throw std::runtime_error(error.to_string());
        }

        const auto buffer = encoding->buffer();

        return { buffer.begin(), buffer.end() };
	}
	catch (const std::exception& e)
	{
		LOG_ERR("unable to compile script: '{}'", e.what());

		return { };
	}
}

std::vector<std::uint8_t> script::compile_from_path(const std::string& script_path)
{
    const auto raw_script_contents = sys::fs::read_from_disk(script_path);

    if (raw_script_contents.empty())
    {
        LOG_ERR("unable to read script file");

        return { };
    }

    const std::string script_contents(reinterpret_cast<const char*>(raw_script_contents.data()),
                                      raw_script_contents.size());

    return compile(script_contents);
}
