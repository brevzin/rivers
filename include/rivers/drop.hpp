#ifndef RIVERS_DROP_HPP
#define RIVERS_DROP_HPP

#include <rivers/core.hpp>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// drop: takes a (RiverOf<T> r, int n) and produces a new RiverOf<T> which
// includes all but the first n elements of r (or no elements,
// if r doesn't have n elements)
////////////////////////////////////////////////////////////////////////////

template <River R>
struct Drop : RiverBase<Drop<R>>
{
private:
    R base;
    int n;
    int i = 0;

public:
    using reference = reference_t<R>;

    constexpr Drop(R base, int n) : base(std::move(base)), n(n) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return base.while_([&](reference elem){
            if (i++ < n) {
                return true;
            } else {
                return std::invoke(pred, elem);
            }
        });
    }

    void reset() requires ResettableRiver<R>
    {
        base.reset();
        i = 0;
    }
};

struct {
    template <River R>
    constexpr auto operator()(R&& r, int n) const {
        return Drop(RVR_FWD(r), n);
    }
} inline constexpr drop;

template <typename Derived>
constexpr auto RiverBase<Derived>::drop(int n) & {
    return Drop(self(), n);
}

template <typename Derived>
constexpr auto RiverBase<Derived>::drop(int n) && {
    return Drop(RVR_FWD(self()), n);
}

}

#endif
