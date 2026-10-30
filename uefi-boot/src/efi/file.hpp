#pragma once
extern "C" {
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
}

#include "obj.hpp"
#include "status.hpp"
#include <expected.hpp>
#include <optional.hpp>
#include <string_view.hpp>
#include <span.hpp>
#include <cstdint>
#include <cstddef>

namespace efi
{
	class file : public obj_wrap<EFI_FILE_PROTOCOL*>
	{
	public:
		using obj_wrap::obj_wrap;

		using mode_base_t = std::uint64_t;
		using attr_base_t = std::uint64_t;

		// owns the handle, so it is move-only
		file(const file&) = delete;
		file& operator=(const file&) = delete;

		file(file&&) noexcept = default;
		file& operator=(file&& other) noexcept;

		~file();

		// matches the spec
		struct info
		{
			std::uint64_t size_of_info;
			std::uint64_t file_size;
			std::uint64_t physical_size;
			EFI_TIME create_time;
			EFI_TIME last_access_time;
			EFI_TIME modification_time;
			std::uint64_t attribute;
			CHAR16 file_name_raw[1];

			[[nodiscard]] cstd::wzstring_view file_name() const noexcept
			{
				return cstd::wzstring_view{ reinterpret_cast<const wchar_t*>(file_name_raw) };
			}
		};

		struct info_holder
		{
			cstd::vector<std::uint8_t> buf;

			[[nodiscard]] info* operator->() noexcept
			{
				return reinterpret_cast<info*>(buf.data());
			}

			[[nodiscard]] const info* operator->() const noexcept
			{
				return reinterpret_cast<const info*>(buf.data());
			}
		};

		enum mode : mode_base_t
		{
			mode_read = EFI_FILE_MODE_READ,	
			mode_write = EFI_FILE_MODE_WRITE,	
			mode_create = EFI_FILE_MODE_CREATE,

			mode_read_write = mode_read | mode_write,
			mode_all = mode_read | mode_write | mode_create
		};

		enum attr : attr_base_t
		{
			attr_read_only = EFI_FILE_READ_ONLY,
			attr_hidden = EFI_FILE_HIDDEN,
			attr_file_system = EFI_FILE_SYSTEM,
			attr_directory = EFI_FILE_DIRECTORY,
			attr_archive = EFI_FILE_ARCHIVE
		};

		[[nodiscard]] cstd::expected<file, status> open(cstd::wzstring_view name,
		                                                mode_base_t mode = mode_all,
		                                                attr_base_t attr = attr_file_system) const noexcept;

		status read(void* buf, std::size_t size) const noexcept;
		status read(cstd::span<std::uint8_t> buf) const noexcept;
		[[nodiscard]] cstd::expected<cstd::vector<std::uint8_t>, status> read() const noexcept;

		status write(const void* buf, std::size_t size) const noexcept;
		status write(cstd::span<const std::uint8_t> buf) noexcept;

		status close() noexcept;
		status delete_() noexcept;

		[[nodiscard]] std::size_t size() const noexcept;

		[[nodiscard]] cstd::expected<info_holder, status> get_info() const noexcept;
		status set_info(const info_holder& info) noexcept;

		// searches all volumes
		static cstd::optional<file> find(cstd::wzstring_view path, mode mode = mode_all, attr attr = attr_file_system);
	};

	class file_system : public obj_wrap<EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*>
	{
	public:
		using obj_wrap::obj_wrap;

		[[nodiscard]] cstd::expected<file, status> volume() const noexcept;

		// calls fn(file_system& fs) for every file system the firmware exposes,
		// stopping early if fn returns true. handles without the protocol are skipped
		template <class Fn>
		static status foreach(Fn&& fn) noexcept
		{
			EFI_HANDLE* handles = nullptr;
			UINTN count = 0;

			status stat = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid,
			                                            nullptr, &count, &handles);

			if (!stat)
				return stat;

			for (UINTN i = 0; i < count; i++)
			{
				EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* native = nullptr;

				stat = gBS->HandleProtocol(handles[i], &gEfiSimpleFileSystemProtocolGuid,
					reinterpret_cast<void**>(&native));

				if (!stat)
					continue;

				file_system fs{ native };

				if (fn(fs))
					break;
			}

			gBS->FreePool(handles);

			return success;
		}

		// calls fn(file& root) for every volume, stopping early if fn returns true.
		// volumes that fail to open are skipped
		template <class Fn>
		static status foreach_vol(Fn&& fn) noexcept
		{
			return foreach([&fn](file_system& fs)
			{
				auto vol = fs.volume();

				if (!vol)
					return false;

				return static_cast<bool>(fn(*vol));
			});
		}
	};
}
