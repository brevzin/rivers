#ifndef RIVERS_FROM_CPP_HPP
#define RIVERS_FROM_CPP_HPP

#include <rivers/core.hpp>
#include <ranges>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// Converting a C++ Range to a River
////////////////////////////////////////////////////////////////////////////
template <std::ranges::input_range R>
struct FromCpp : RiverBase<FromCpp<R>>
{
private:
    R base;

public:
    static constexpr bool multipass = std::ranges::forward_range<R>;
    using reference = std::ranges::range_reference_t<R>;
    using value_type = std::ranges::range_value_t<R>;

    constexpr FromCpp(R&& r) : base(std::move(r)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        auto it = std::ranges::begin(base);
        auto end = std::ranges::end(base);
        for (; it != end; ++it) {
            if (not std::invoke(pred, *it)) {
                return false;
            }
        }
        return true;
    }
};

struct {
    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr auto operator()(I first, S last) const {
        return FromCpp(std::ranges::subrange(std::move(first), std::move(last)));
    }

    template <std::ranges::input_range R>
    constexpr auto operator()(R&& r) const {
        using U = std::remove_cvref_t<R>;
        if constexpr (std::ranges::view<R>) {
            return FromCpp<U>(RVR_FWD(r));
        } else if constexpr (std::is_lvalue_reference_v<R>) {
            return FromCpp(std::ranges::ref_view(r));
        } else {
            return FromCpp(RVR_FWD(r));
        }
    }
} inline constexpr from_cpp;

}

#endif
