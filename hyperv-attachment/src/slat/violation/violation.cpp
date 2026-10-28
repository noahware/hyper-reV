#include "violation.hpp"
#include "../cr3/cr3.hpp"
#include "../hook/info.hpp"
#include "../hook/split.hpp"
#include "../../arch/current.hpp"

bool slat::violation::handle() noexcept
{
	const auto violation = arch::slat_violation();

	if (!violation.caused_by_translation)
		return false;

	if ((violation.access & page_perm::execute) == page_perm::none)
		return false;

	const auto page_gpa = violation.gpa & ~page_mask;

	if (arch::slat_cr3().flags != cr3::hyperv_cr3.flags)
	{
		hook::handle_page_split(page_gpa);

		arch::set_slat_cr3(cr3::hyperv_cr3);
		arch::flush_slat_cache();
		return true;
	}

	cstd::shared_lock guard(hook::mtx);

	for (const auto& hook : hook::list)
	{
		if ((hook.gpa & ~page_mask) != page_gpa)
			continue;

		arch::set_slat_cr3(cr3::shadow_cr3);
		arch::flush_slat_cache();
		return true;
	}

	return false;
}
