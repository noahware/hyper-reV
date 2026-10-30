#include "file.hpp"

extern "C" {
#include <Library/DebugLib.h>
}

efi::file& efi::file::operator=(file&& other) noexcept
{
	if (this != &other)
	{
		if (native_)
		{
			close();
		}

		obj_wrap::operator=(static_cast<obj_wrap&&>(other));
	}

	return *this;
}

efi::file::~file()
{
	if (native_)
	{
		close();
	}
}

cstd::expected<efi::file, efi::status> efi::file::open(const cstd::wzstring_view name, const mode_base_t mode,
                                                       const attr_base_t attr) const noexcept
{
	EFI_FILE_PROTOCOL* root;

	const auto raw_name = const_cast<CHAR16*>(reinterpret_cast<const CHAR16*>(name.c_str()));
	const status stat = native_->Open(native_, &root, raw_name, mode, attr);

	if (!stat)
		return cstd::unexpected(stat);

	return file{ root };
}

efi::status efi::file::read(void* const buf, const std::size_t size) const noexcept
{
	UINTN s = static_cast<UINTN>(size);

	return status{ native_->Read(native_, &s, buf) };
}

efi::status efi::file::read(const cstd::span<std::uint8_t> buf) const noexcept
{
	return read(buf.data(), buf.size());
}

cstd::expected<cstd::vector<std::uint8_t>, efi::status> efi::file::read() const noexcept
{
	cstd::vector<std::uint8_t> buf(size());

	const status stat = read(buf);

	if (!stat)
		return cstd::unexpected(stat);

	return buf;
}

efi::status efi::file::write(const void* const buf, const std::size_t size) const noexcept
{
	UINTN s = static_cast<UINTN>(size);

	return status{ native_->Write(native_, &s, const_cast<void*>(buf)) };
}

efi::status efi::file::write(const cstd::span<const std::uint8_t> buf) noexcept
{
	return write(buf.data(), buf.size());
}

efi::status efi::file::close() noexcept
{
	const status stat{ native_->Close(native_) };

	// the handle is invalid after Close, even on failure
	native_ = nullptr;

	return stat;
}

efi::status efi::file::delete_() noexcept
{
	const status stat{ native_->Delete(native_) };

	// the handle is invalid after Delete, even on failure
	native_ = nullptr;

	return stat;
}

std::size_t efi::file::size() const noexcept
{
	const auto info = get_info();

	ASSERT(info.has_value());

	return (*info)->file_size;
}

cstd::expected<efi::file::info_holder, efi::status> efi::file::get_info() const noexcept
{
	EFI_GUID info_type = EFI_FILE_INFO_ID;

	UINTN size = 0;

	status stat = native_->GetInfo(native_, &info_type, &size, nullptr);

	if (stat != buffer_too_small)
	{
		return cstd::unexpected(stat);
	}

	cstd::vector<std::uint8_t> buf(static_cast<std::size_t>(size));

	stat = native_->GetInfo(native_, &info_type, &size, buf.data());

	return info_holder{ cstd::move(buf) };
}

efi::status efi::file::set_info(const info_holder& info) noexcept
{
	EFI_GUID info_type = EFI_FILE_INFO_ID;

	const auto buf = const_cast<std::uint8_t*>(info.buf.data());

	return native_->SetInfo(native_, &info_type, info.buf.size(), buf);
}

cstd::optional<efi::file> efi::file::find(const cstd::wzstring_view path, const mode mode, const attr attr)
{
	cstd::optional<file> result = cstd::nullopt;

	file_system::foreach_vol(
		[&](file& root)
		{
			auto curr = root.find(path, mode, attr);

			if (!curr)
				return false;

			result = cstd::move(*curr);

			return true;
		}
	);

	return result;
}

cstd::expected<efi::file, efi::status> efi::file_system::volume() const noexcept
{
	EFI_FILE_PROTOCOL* root;

	const status status = native_->OpenVolume(native_, &root);

	if (!status)
		return cstd::unexpected(status);

	return file{ root };
}
