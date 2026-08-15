#pragma once
#include <keystone/keystone.h>
#include <expected>
#include <span>

class keystone_error
{
public:
    explicit keystone_error(const ks_err value)
			:   value_(value) { }

    [[nodiscard]] std::string to_string() const
    {
        return ks_strerror(value_);
    }

    explicit operator bool() const noexcept
    {
        return value_ != KS_ERR_OK;
    }

protected:
    ks_err value_ = KS_ERR_OK;
};

class encoded_assembly
{
public:
    explicit encoded_assembly(const std::span<std::uint8_t> buffer)
			:   buffer_(buffer) { }

    ~encoded_assembly()
    {
	    if (buffer_.data())
	    {
            ks_free(buffer_.data());
	    }
    }

    encoded_assembly(const encoded_assembly&) = delete;
    encoded_assembly& operator=(const encoded_assembly&) = delete;

    encoded_assembly(encoded_assembly&& right) noexcept
			:   buffer_(right.buffer_)
    {
        right.buffer_ = { };
    }

    encoded_assembly& operator=(encoded_assembly&& right) noexcept
    {
        if (this != &right)
        {
            buffer_ = right.buffer_;

            right.buffer_ = { };
        }

        return *this;
    }

    [[nodiscard]] std::span<std::uint8_t> buffer() noexcept
    {
        return buffer_;
    }

    [[nodiscard]] std::span<const std::uint8_t> buffer() const noexcept
    {
        return buffer_;
    }

protected:
    std::span<std::uint8_t> buffer_;
};

class keystone_assembler
{
public:
    using native_type = ks_engine*;
    using symbol_resolver_fn = bool(*)(const char*, std::uint64_t*);

    explicit keystone_assembler(const native_type engine)
			:   engine_(engine) { }

    ~keystone_assembler()
    {
	    if (engine_)
	    {
            ks_close(engine_);
	    }
    }

    keystone_assembler(const keystone_assembler&) = delete;
    keystone_assembler& operator=(const keystone_assembler&) = delete;

    keystone_assembler(keystone_assembler&& right) noexcept
			:   engine_(right.engine_)
    {
        right.engine_ = { };
    }

    keystone_assembler& operator=(keystone_assembler&& right) noexcept
    {
        if (this != &right)
        {
            engine_ = right.engine_;

            right.engine_ = { };
        }

        return *this;
    }

    keystone_error set_syntax(const std::int32_t syntax)
    {
        return keystone_error{ ks_option(engine_, KS_OPT_SYNTAX, syntax) };
    }

    [[nodiscard]] std::expected<encoded_assembly, keystone_error> assemble(const std::string& assembly) const
    {
        std::uint8_t* buffer = nullptr;
        std::size_t size = 0;

        std::size_t stat_count = 0;

        if (ks_asm(engine_, assembly.c_str(), 0, &buffer, &size, &stat_count))
        {
            const keystone_error error{ ks_errno(engine_) };

            return std::unexpected(error);
        }

        return encoded_assembly{ std::span(buffer, size) };
    }

	operator ks_engine*() const noexcept
    {
        return native_handle();
    }

    [[nodiscard]] ks_engine* native_handle() const noexcept
    {
        return engine_;
    }

    static std::expected<keystone_assembler, keystone_error> create_safe(const ks_arch arch, const std::int32_t mode)
    {
        ks_engine* engine = nullptr;

        if (const keystone_error error{ ks_open(arch, mode, &engine) })
        {
            return std::unexpected(error);
        }

        return keystone_assembler{ engine };
    }

    static keystone_assembler create(const ks_arch arch, const std::int32_t mode)
    {
        auto creation = create_safe(arch, mode);

        return std::move(creation.value());
    }

protected:
    ks_engine* engine_ = nullptr;
};

