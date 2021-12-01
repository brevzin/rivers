#ifndef RIVERS_FROM_HPP
#define RIVERS_FROM_HPP

#include <ranges>
#include <rivers/core.hpp>
#include <rivers/tag_invoke.hpp>


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// Converting a C++ Range to a River
// * from(r)           for a range
// * from(first, last) for an iterator/sentinel pair
// The resulting river is resettable if the source range is forward or better
////////////////////////////////////////////////////////////////////////////
template <std::ranges::input_range R>
struct From : RiverBase<From<R>>
{
private:
    R base;
    std::ranges::iterator_t<R> it = std::ranges::begin(base);
    std::ranges::sentinel_t<R> end = std::ranges::end(base);

public:
    using reference = std::ranges::range_reference_t<R>;
    using value_type = std::ranges::range_value_t<R>;

    constexpr From(R&& r) : base(std::move(r)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        while (it != end) {
            RVR_SCOPE_EXIT { ++it; };
            if (not std::invoke(pred, *it)) {
                return false;
            }
        }
        return true;
    }

    void reset() requires std::ranges::forward_range<R> {
        it = std::ranges::begin(base);
    }
};

struct from_fn {
    template <std::ranges::input_range R>
    friend constexpr auto tag_invoke(from_fn, R&& r) {
        using U = std::remove_cvref_t<R>;
        if constexpr (std::ranges::view<R>) {
            return From<U>(RVR_FWD(r));
        } else if constexpr (std::is_lvalue_reference_v<R>) {
            return From(std::ranges::ref_view(r));
        } else {
            return From(RVR_FWD(r));
        }
    }

    template <typename R>
        requires tag_invocable<from_fn, R>
    constexpr auto operator()(R&& r) const {
        return rvr::tag_invoke(*this, RVR_FWD(r));
    }

    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr auto operator()(I first, S last) const {
        return From(std::ranges::subrange(std::move(first), std::move(last)));
    }
} inline constexpr from;

}

#endif
