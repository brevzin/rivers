#ifndef RANGES_FILTER_HPP
#define RANGES_FILTER_HPP

#include <rivers/core.hpp>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// filter: takes a (RiverOf<T>, T -> bool) and produces a new RiverOf<T>
// containing only the elements that satisfy the predicate
////////////////////////////////////////////////////////////////////////////
template <River R, typename P>
    requires std::predicate<P&, reference_t<R>>
struct Filter : RiverBase<Filter<R, P>> {
private:
    R base;
    P filter;

public:
    using reference = reference_t<R>;

    constexpr Filter(R base, P pred) : base(std::move(base)), filter(std::move(pred)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return base.while_([&](reference e){
            if (std::invoke(filter, e)) {
                return std::invoke(pred, RVR_FWD(e));
            } else {
                // if we don't satisfy our internal predicate, we keep going
                return true;
            }
        });
    }

    void reset() requires ResettableRiver<R> {
        base.reset();
    }
};

struct {
    template <River R, typename P = std::identity>
        requires std::predicate<P&, reference_t<R>&>
    constexpr auto operator()(R&& base, P&& pred = {}) const {
        return Filter(RVR_FWD(base), RVR_FWD(pred));
    }
} inline constexpr filter;

template <typename Derived>
template <typename P> requires std::predicate<P&, reference_t<Derived>&>
constexpr auto RiverBase<Derived>::filter(P&& pred) & {
    return Filter(self(), RVR_FWD(pred));
}

template <typename Derived>
template <typename P> requires std::predicate<P&, reference_t<Derived>&>
constexpr auto RiverBase<Derived>::filter(P&& pred) && {
    return Filter(RVR_FWD(self()), RVR_FWD(pred));
}

}

#endif
