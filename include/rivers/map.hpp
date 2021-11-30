#ifndef RANGES_MAP_HPP
#define RANGES_MAP_HPP

#include <rivers/core.hpp>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// map: takes a (RiverOf<T>, T -> U) and produces a RiverOf<U>
////////////////////////////////////////////////////////////////////////////
template <River R, typename F>
    requires std::regular_invocable<F&, reference_t<R>>
struct Map : RiverBase<Map<R, F>> {
private:
    R base;
    F f;

public:
    using reference = std::invoke_result_t<F&, reference_t<R>>;

    Map(R base, F f) : base(std::move(base)), f(std::move(f)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return base.while_([&](reference e){
            return std::invoke(pred, std::invoke(f, RVR_FWD(e)));
        });
    }

    void reset() requires ResettableRiver<R> {
        base.reset();
    }
};

struct {
    template <River R, typename F>
        requires std::regular_invocable<F&, reference_t<R>>
    constexpr auto operator()(R&& base, F&& f) const {
        return Map(RVR_FWD(base), RVR_FWD(f));
    }
} inline constexpr map;

template <typename Derived>
template <typename F> requires std::invocable<F&, reference_t<Derived>>
constexpr auto RiverBase<Derived>::map(F&& f) & {
    return Map(self(), RVR_FWD(f));
}

template <typename Derived>
template <typename F> requires std::invocable<F&, reference_t<Derived>>
constexpr auto RiverBase<Derived>::map(F&& f) && {
    return Map(RVR_FWD(self()), RVR_FWD(f));
}

}

#endif
