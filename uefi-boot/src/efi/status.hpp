#pragma once
extern "C" {
#include <Uefi.h>
}

namespace efi
{
	class status
	{
	public:
		using value_type = EFI_STATUS;

		constexpr status() noexcept = default;
		constexpr status(const value_type value) noexcept
			:	value_(value) { }

		[[nodiscard]] constexpr operator bool() const noexcept
		{
			return !EFI_ERROR(value_);
		}

		[[nodiscard]] constexpr value_type value() const noexcept
		{
			return value_;
		}

		[[nodiscard]] constexpr bool operator==(const status other) const noexcept
		{
			return value_ == other.value_;
		}

	protected:
		value_type value_ = EFI_SUCCESS;
	};

	inline constexpr status success = EFI_SUCCESS;
	inline constexpr status buffer_too_small = EFI_BUFFER_TOO_SMALL;
	inline constexpr status not_found = EFI_NOT_FOUND;
	inline constexpr status out_of_resources = EFI_OUT_OF_RESOURCES;
	inline constexpr status no_media = EFI_NO_MEDIA;
	inline constexpr status aborted = EFI_ABORTED;
}
