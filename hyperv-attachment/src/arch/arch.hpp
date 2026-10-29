#pragma once
#include <type_traits.hpp>
#include <ia32.hpp>
#include "defs.hpp"
#include <guest_regs.hpp>
#include "exit_reason.hpp"

struct boot_info;

template <typename T>
concept arch_backend = requires(const boot_info& bi,
	typename T::slat_pte pte, typename T::slat_pde pde,
	typename T::slat_pdpte pdpte, typename T::slat_pml4e pml4e,
	typename T::slat_pde_2mb pde_2mb, typename T::slat_pdpte_1gb pdpte_1gb) {
	{ T::init(bi)                                                         } -> cstd::same_as<void>;
	{ T::guest_regs(std::uint64_t{}, std::uint64_t{})                     } -> cstd::same_as<struct guest_regs>;
	{ T::set_guest_regs(std::uint64_t{}, std::uint64_t{}, ::guest_regs{}) } -> cstd::same_as<void>;

	{ T::guest_cr3()              } -> cstd::same_as<::cr3>;
	{ T::rip()                    } -> cstd::same_as<std::uint64_t>;
	{ T::set_rip(std::uint64_t{}) } -> cstd::same_as<void>;
	{ T::advance_rip()            } -> cstd::same_as<void>;

	{ T::exit_reason()            } -> cstd::same_as<enum exit_reason>;

	typename T::slat_pte;
	typename T::slat_pde;
	typename T::slat_pdpte;
	typename T::slat_pml4e;
	{ T::slat_is_present(typename T::slat_pte{})   } -> cstd::same_as<bool>;
	{ T::slat_is_present(typename T::slat_pde{})    } -> cstd::same_as<bool>;
	{ T::slat_is_present(typename T::slat_pdpte{})  } -> cstd::same_as<bool>;
	{ T::slat_is_present(typename T::slat_pml4e{})  } -> cstd::same_as<bool>;

	{ T::slat_perm(typename T::slat_pte{})    } -> cstd::same_as<page_perm>;
	{ T::slat_perm(typename T::slat_pde{})    } -> cstd::same_as<page_perm>;
	{ T::slat_perm(typename T::slat_pdpte{})   } -> cstd::same_as<page_perm>;
	{ T::slat_perm(typename T::slat_pml4e{})   } -> cstd::same_as<page_perm>;

	{ T::slat_set_perm(pte, page_perm{})    } -> cstd::same_as<void>;
	{ T::slat_set_perm(pde, page_perm{})    } -> cstd::same_as<void>;
	{ T::slat_set_perm(pdpte, page_perm{})  } -> cstd::same_as<void>;
	{ T::slat_set_perm(pml4e, page_perm{})  } -> cstd::same_as<void>;

	{ T::slat_is_large(typename T::slat_pde{})    } -> cstd::same_as<bool>;
	{ T::slat_is_large(typename T::slat_pdpte{})   } -> cstd::same_as<bool>;

	typename T::slat_pde_2mb;
	typename T::slat_pdpte_1gb;
	{ T::slat_init_split_pte(pte, pde_2mb, std::uint64_t{})       } -> cstd::same_as<void>;
	{ T::slat_init_split_pde(pde_2mb, pdpte_1gb, std::uint64_t{}) } -> cstd::same_as<void>;
	{ T::slat_set_table_pde(pde, std::uint64_t{})                 } -> cstd::same_as<void>;
	{ T::slat_set_table_pdpte(pdpte, std::uint64_t{})             } -> cstd::same_as<void>;
	{ T::slat_init_merge_pde(pde_2mb, pte)                        } -> cstd::same_as<void>;

	{ T::slat_cr3()               } -> cstd::same_as<::cr3>;
	{ T::set_slat_cr3(::cr3{})    } -> cstd::same_as<void>;
	{ T::flush_slat_cache()       } -> cstd::same_as<void>;

	{ T::slat_violation()         } -> cstd::same_as<struct ::slat_violation>;
};
