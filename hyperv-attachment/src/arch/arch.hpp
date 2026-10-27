#pragma once
#include <type_traits.hpp>
#include "guest_regs.hpp"
#include "exit_reason.hpp"

struct boot_info;

template <typename T>
concept arch_backend = requires(const boot_info& bi) {
	{ T::init(bi)                                                         } -> cstd::same_as<void>;
	{ T::guest_regs(std::uint64_t{}, std::uint64_t{})                     } -> cstd::same_as<struct guest_regs>;
	{ T::set_guest_regs(std::uint64_t{}, std::uint64_t{}, ::guest_regs{}) } -> cstd::same_as<void>;

	{ T::rip()                    } -> cstd::same_as<std::uint64_t>;
	{ T::set_rip(std::uint64_t{}) } -> cstd::same_as<void>;
	{ T::advance_rip()            } -> cstd::same_as<void>;

	{ T::exit_reason()            } -> cstd::same_as<enum exit_reason>;
};
