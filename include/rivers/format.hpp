#ifndef RIVERS_FORMAT_HPP
#define RIVERS_FORMAT_HPP

#if __has_include(<fmt/format.h>)
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <rivers/core.hpp>

namespace rvr {

template <typename T>
struct wrapped_debug_formatter : fmt::formatter<T> {
    bool use_debug = false;

    constexpr void format_as_debug() {
        use_debug = true;
    }

    template <typename FormatContext>
    constexpr auto format(T const& value, FormatContext& ctx) const {
        if constexpr (fmt::detail::is_std_string_like<T>::value or std::is_same_v<T, char>) {
            if (use_debug) {
                return fmt::detail::write_range_entry<char>(ctx.out(), value);
            }
        }

        return fmt::formatter<T>::format(value, ctx);
    }
};

template <typename V>
struct river_formatter {
    fmt::detail::dynamic_format_specs<char> specs;
    wrapped_debug_formatter<V> underlying;
    bool is_debug = false;
    bool use_brackets = true;

    template <typename Iterator, typename Handler>
    constexpr auto parse_range_specs(Iterator begin, Iterator end, Handler&& handler) {
        // parse_align isn't quite right because we need to escape the
        // : if that's the desired fill
        if (*begin == '\\') {
            FMT_ASSERT(begin[1] == ':', "expected colon");
            begin = fmt::detail::parse_align(begin + 1, end, handler);
            FMT_ASSERT(specs.align != fmt::align::none, "escape but no align?");
            end = std::ranges::find(begin, end, ':');
        } else {
            end = std::ranges::find(begin, end, ':');
            if (begin == end) return begin;
            begin = fmt::detail::parse_align(begin, end, handler);
        }

        if (begin == end) return end;

        begin = fmt::detail::parse_width(begin, end, handler);
        if (begin == end) return begin;

        if (*begin == '?') {
            is_debug = true;
            ++begin;
            if (begin == end) return begin;
        }

        if (begin != end && *begin != '}') {
            switch (*begin++) {
            case 's':
                specs.type = fmt::presentation_type::string;
                break;
            case 'e':
                use_brackets = false;
                break;
            default:
                throw fmt::format_error("unknown type");
            }
        }

        return begin;
    }

    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        auto begin = ctx.begin();
        auto end = ctx.end();
        if (begin == end || *begin == '}') {
            if constexpr (requires { underlying.format_as_debug(); }) {
                underlying.format_as_debug();
            }
            return begin;
        }

        using handler_type = fmt::detail::dynamic_specs_handler<ParseContext>;
        auto checker =
            fmt::detail::specs_checker<handler_type>(handler_type(specs, ctx), fmt::detail::type::none_type);
        auto it = parse_range_specs(ctx.begin(), ctx.end(), checker);

        if (it == ctx.end() || *it == '}') {
            if constexpr (requires { underlying.format_as_debug(); }) {
                underlying.format_as_debug();
            }
        } else {
            ++it;
        }

        ctx.advance_to(it);
        return underlying.parse(ctx);
    }

    template <typename R, typename FormatContext>
        requires std::same_as<std::remove_cvref_t<reference_t<R>>, V>
    constexpr auto format(R&& r, FormatContext& ctx) const -> typename FormatContext::iterator
    {
        if (specs.type == fmt::presentation_type::string) {
            if constexpr (std::same_as<V, char>) {
                auto s = r.into_str();
                if (is_debug) {
                    return fmt::detail::write_range_entry<char>(ctx.out(), s);
                } else {
                    return fmt::detail::write(ctx.out(), fmt::string_view(s), specs);
                }
            } else {
                throw fmt::format_error("wat");
            }
        }

        fmt::memory_buffer buf;
        fmt::basic_format_context<fmt::appender, char>
            bctx(fmt::appender(buf), ctx.args(), ctx.locale());

        auto out = bctx.out();
        if (use_brackets) {
            *out++ = '[';
        }

        bool first = true;
        r.for_each([&](auto&& elem){
            if (not first) {
                *out++ = ',';
                *out++ = ' ';
            } else {
                first = false;
            }

            // have to format every element via the underlying formatter
            bctx.advance_to(std::move(out));
            out = underlying.format(elem, bctx);
        });

        if (use_brackets) {
            *out++ = ']';
        }

        return fmt::detail::write(ctx.out(), fmt::string_view(buf.data(), buf.size()), specs);
    }
};

}

template <rvr::River R>
struct fmt::formatter<R>
    : rvr::river_formatter<std::remove_cvref_t<rvr::reference_t<R>>>
{
    template <typename FormatContext>
    constexpr auto format(R& r, FormatContext& ctx) {
        return rvr::river_formatter<std::remove_cvref_t<rvr::reference_t<R>>>::format(r, ctx);
    }
};

#endif

#endif
