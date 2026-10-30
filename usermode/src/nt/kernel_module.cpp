#include "kernel_module.hpp"
#include "priv.hpp"
#include "query.hpp"

namespace nt
{
	struct raw_kernel_module
	{
		void* section;
		void* mapped_base;
		void* image_base;
		std::uint32_t image_size;
		std::uint32_t flags;
		std::uint16_t load_order_index;
		std::uint16_t init_order_index;
		std::uint16_t load_count;
		std::uint16_t offset_to_file_name;
		std::uint8_t full_path[256];

		[[nodiscard]] kernel_module_info to_info() const noexcept
		{
			const auto* const p = reinterpret_cast<const char*>(full_path);

			return
			{
				.base = reinterpret_cast<std::uintptr_t>(image_base),
				.size = image_size,
				.name = p + offset_to_file_name,
				.path = p,
			};
		}
	};
}

namespace
{
	struct raw_header
	{
		std::uint32_t count;
		nt::raw_kernel_module modules[1];
	};

	const raw_header* header_of(const std::vector<std::uint8_t>& buf) noexcept
	{
		return reinterpret_cast<const raw_header*>(buf.data());
	}
}

nt::kernel_modules::kernel_modules() noexcept
{
	const auto dbg = privilege::debug();

	if (!dbg)
		return;

	buf_ = query_system_info(system_module_information_class);
}

std::size_t nt::kernel_modules::count() const noexcept
{
	if (buf_.empty())
		return 0;

	return header_of(buf_)->count;
}

nt::kernel_module_info nt::kernel_modules::operator[](const std::size_t i) const noexcept
{
	return header_of(buf_)->modules[i].to_info();
}

nt::kernel_module_info nt::kernel_modules::find(const std::string_view name) const noexcept
{
	for (const auto m : *this)
	{
		if (m.name == name)
			return m;
	}

	return {};
}

nt::kernel_modules::iterator nt::kernel_modules::begin() const noexcept
{
	if (buf_.empty())
		return { nullptr };

	return { header_of(buf_)->modules };
}

nt::kernel_modules::iterator nt::kernel_modules::end() const noexcept
{
	if (buf_.empty())
		return { nullptr };

	const auto* const h = header_of(buf_);

	return { h->modules + h->count };
}

nt::kernel_module_info nt::kernel_modules::iterator::operator*() const noexcept
{
	return ptr->to_info();
}

nt::kernel_modules::iterator& nt::kernel_modules::iterator::operator++() noexcept
{
	++ptr;
	return *this;
}
