#pragma once
#include <cstdint>

struct guest_regs
{
	std::uint64_t rax, rcx, rdx, rbx;
	std::uint64_t rsp;
	std::uint64_t rbp, rsi, rdi;
	std::uint64_t r8, r9, r10, r11;
	std::uint64_t r12, r13, r14, r15;
};
