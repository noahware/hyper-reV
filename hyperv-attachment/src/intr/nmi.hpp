#pragma once

namespace intr::nmi
{
	void init() noexcept;
	void process() noexcept;
	void send_all_but_self() noexcept;
}
