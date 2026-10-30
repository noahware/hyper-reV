#pragma once
extern "C" {
#include <Uefi.h>
}

namespace efi
{
	template <class T>
	class obj_wrap
	{
	public:
		obj_wrap() noexcept = default;
		obj_wrap(T native) noexcept
			:	native_(native) { }

		obj_wrap(const obj_wrap&) noexcept = default;
		obj_wrap& operator=(const obj_wrap&) noexcept = default;

		// a moved-from wrapper no longer refers to the object
		obj_wrap(obj_wrap&& other) noexcept
			:	native_(other.native_)
		{
			other.native_ = nullptr;
		}

		obj_wrap& operator=(obj_wrap&& other) noexcept
		{
			if (this != &other)
			{
				native_ = other.native_;
				other.native_ = nullptr;
			}

			return *this;
		}

		[[nodiscard]] T native() const noexcept
		{
			return native_;
		}

	protected:
		T native_ = nullptr;
	};

	using handle = EFI_HANDLE;

	using handle_obj = obj_wrap<handle>;
}
