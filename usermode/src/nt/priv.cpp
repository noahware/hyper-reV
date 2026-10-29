#include "priv.hpp"

#include <Windows.h>

extern "C" NTSTATUS NTAPI RtlAdjustPrivilege(ULONG privilege, BOOLEAN enable, BOOLEAN current_thread,
                                             PBOOLEAN previous_enabled);

nt::privilege::privilege(const std::uint32_t priv) noexcept
	: priv_(priv)
{
	BOOLEAN prev = 0;

	if (RtlAdjustPrivilege(priv_, TRUE, FALSE, &prev) >= 0)
	{
		prev_state_ = prev;
		acquired_ = true;
	}
}

nt::privilege::~privilege() noexcept
{
	if (!acquired_)
		return;

	BOOLEAN discard = 0;
	RtlAdjustPrivilege(priv_, prev_state_, FALSE, &discard);
}
