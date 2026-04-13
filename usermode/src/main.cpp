#include <iostream>
#include <thread>
#include <string>

#include <Windows.h>

#include "commands/commands.h"
#include "hook/hook.h"
#include "logs/logs.h"
#include "system/system.h"

std::int32_t main()
{
	logs::set_up();

	if (sys::set_up() == 0)
	{
		std::system("pause");

		return 1;
	}

	sys::kernel::parse_modules();

	while (true)
	{
		LOG_PRINTNOLINE("> ");

		std::string command;

		std::getline(std::cin, command);

		if (command == "exit")
		{
			break;
		}

		commands::process(command);

		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}

	sys::clean_up();

	return 0;
}
