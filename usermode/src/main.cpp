#include "cmd/cmd.hpp"
#include "sys/sys.hpp"
#include "util/log.hpp"

int main()
{
	if (!sys::init())
	{
		LOG_ERR("sys::init failed");
		return 1;
	}

	cmd::run();
	return 0;
}
