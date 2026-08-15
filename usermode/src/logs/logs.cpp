#include "logs.h"

void logs::set_up()
{
	spdlog::set_pattern("[%^%l%$] %v");
}
