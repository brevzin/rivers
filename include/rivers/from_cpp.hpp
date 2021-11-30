#ifndef RIVERS_FROM_CPP_HPP
#define RIVERS_FROM_CPP_HPP

#include <ranges>
#include <rivers/core.hpp>
#include <rivers/tag_invoke.hpp>


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// Converting a C++ Range to a River
////////////////////////////////////////////////////////////////////////////
template <std::ranges::input_range R>
struct FromCpp : RiverBase<FromCpp<R>>
{
private:
    R base;
    std::ranges::iterator_t<R> it = std::ranges::begin(base);
    std::ranges::sentinel_t<R> end = std::ranges::end(base);

public:
    using reference = std::ranges::range_reference_t<R>;
    using value_type = std::ranges::range_value_t<R>;

    constexpr FromCpp(R&& r) : base(std::move(r)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        while (it != end) {
            RVR_SCOPE_EXIT { ++it; };
            if (not std::invoke(pred, *it)) {
                return false;
            }
        }
        return true;
    }
};

struct from_cpp_fn {
    template <std::ranges::input_range R>
    friend constexpr auto tag_invoke(from_cpp_fn, R&& r) {
        using U = std::remove_cvref_t<R>;
        if constexpr (std::ranges::view<R>) {
            return FromCpp<U>(RVR_FWD(r));
        } else if constexpr (std::is_lvalue_reference_v<R>) {
            return FromCpp(std::ranges::ref_view(r));
        } else {
            return FromCpp(RVR_FWD(r));
        }
    }

    template <typename R>
        requires tag_invocable<from_cpp_fn, R>
    constexpr auto operator()(R&& r) const {
        return rvr::tag_invoke(*this, RVR_FWD(r));
    }

    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr auto operator()(I first, S last) const {
        return FromCpp(std::ranges::subrange(std::move(first), std::move(last)));
    }
} inline constexpr from_cpp;

}

#endif
