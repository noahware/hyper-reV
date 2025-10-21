#pragma once
#include <structures/trap_frame.h>

union hypercall_info_t;

namespace hypercall
{
uint64_t process(hypercall_info_t hypercall_info, trap_frame_t *trap_frame);
}
