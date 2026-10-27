#pragma once
#include "guest_regs.hpp"
#include "exit_reason.hpp"

template <typename T>
concept arch_backend = requires {
	{ T::guest_regs(std::uint64_t{}, std::uint64_t{})                      } -> std::same_as<struct guest_regs>;
	{ T::set_guest_regs(std::uint64_t{}, std::uint64_t{}, ::guest_regs{}) } -> std::same_as<void>;

	{ T::rip()                    } -> std::same_as<std::uint64_t>;
	{ T::set_rip(std::uint64_t{}) } -> std::same_as<void>;
	{ T::advance_rip()            } -> std::same_as<void>;

	{ T::exit_reason()            } -> std::same_as<enum exit_reason>;
};
