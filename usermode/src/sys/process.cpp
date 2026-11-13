#include "process.hpp"

namespace sys::process
{
    namespace
    {
        std::uint32_t first_mov_disp( const std::uint8_t* fn ) noexcept
        {
            for ( std::uint32_t i = 0; i < 64; ++i )
            {
                if ( fn[ i ] != 0x48 || fn[ i + 1 ] != 0x8B )
                    continue;

                const auto mod = fn[ i + 2 ] >> 6;

                if ( mod == 1 )
                    return fn[ i + 3 ];

                if ( mod == 2 )
                {
                    std::uint32_t d {};
                    memcpy( &d, fn + i + 3, 4 );
                    if ( d >= 0x100 && d <= 0x500 )
                        return d;
                }
            }

            return 0;
        }

        struct context_t
        {
            std::uintptr_t first_eprocess = 0;
            std::uint32_t cr3 = 0x028;
            std::uint32_t pid = 0x1d0;
            std::uint32_t flink = 0x1d8;
            std::uint32_t peb = 0x3f8;
        };

        context_t init_context() noexcept
        {
            context_t ctx {};

            const auto it = kern::modules_list.find( "ntoskrnl.exe" );
            if ( it == kern::modules_list.end() )
                return ctx;

            const auto& exports = it->second.exports;

            const auto read_fn = [ & ] ( const char* name ) -> std::uint32_t
                {
                    const auto exp = exports.find( name );
                    if ( exp == exports.end() )
                        return 0;

                    std::uint8_t bytes[ 64 ] {};
                    for ( std::uint32_t i = 0; i < 64; ++i )
                        bytes[ i ] = kern::read<std::uint8_t>( exp->second + i );

                    return first_mov_disp( bytes );
                };

            if ( const auto v = read_fn( "PsGetProcessId" ) )
                ctx.pid = v;

            if ( const auto v = read_fn( "PsGetNextProcess" ) )
                ctx.flink = v;

            const auto sys_proc = exports.find( "PsInitialSystemProcess" );
            if ( sys_proc != exports.end() )
                ctx.first_eprocess = kern::read<std::uintptr_t>( sys_proc->second );

            return ctx;
        }

        const context_t& ctx() noexcept
        {
            static const context_t c = init_context();
            return c;
        }

        std::uintptr_t find_eprocess( std::uint32_t pid ) noexcept
        {
            const auto& c = ctx();
            if ( !c.first_eprocess )
                return 0;

            std::uintptr_t current = c.first_eprocess;

            for ( int i = 0; i < 1024; ++i )
            {
                if ( kern::read<std::uint64_t>( current + c.pid ) == pid )
                    return current;

                const auto flink = kern::read<std::uintptr_t>( current + c.flink );
                if ( !flink )
                    return 0;

                current = flink - c.flink;

                if ( current == c.first_eprocess )
                    return 0;
            }

            return 0;
        }
    }

    std::uint64_t cr3( const std::uint32_t pid ) noexcept
    {
        const auto& c = ctx();
        const auto e = find_eprocess( pid );
        if ( !e )
            return 0;

        return  kern::read<std::uintptr_t>( e + c.cr3 );
    }

    std::uintptr_t base( const std::uint32_t pid ) noexcept
    {
        const auto& c = ctx();
        const auto  e = find_eprocess( pid );
        if ( !e )
            return 0;

        const auto peb = kern::read<std::uintptr_t>( e + c.peb );
        if ( !peb )
            return 0;

        const auto target_cr3 = kern::read<std::uint64_t>( e + c.cr3 );
        if ( !target_cr3 )
            return 0;

        std::uintptr_t image_base = 0;
        if ( !hc::virt_copy(target_cr3, peb + 0x10, hc::curr_cr3(), 
            reinterpret_cast< std::uint64_t >( &image_base ), sizeof( image_base ) ) )
            return 0;

        return image_base;
    }
}