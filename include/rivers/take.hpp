#ifndef RIVERS_TAKE_HPP
#define RIVERS_TAKE_HPP

#include <rivers/core.hpp>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// take: takes a (RiverOf<T> r, int n) and produces a new RiverOf<T> which
// includes up to the first n elements of r (or fewer, if r doesn't have n elements)
////////////////////////////////////////////////////////////////////////////

template <River R>
struct Take : RiverBase<Take<R>>
{
private:
    R base;
    int n;
    int i = 0;

public:
    using reference = reference_t<R>;

    constexpr Take(R base, int n) : base(std::move(base)), n(n) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        if (i != n) {
            return base.while_([&](reference elem){
                ++i;
                if (not std::invoke(pred, elem) or i == n) {
                    return false;
                } else {
                    return true;
                }
            });
        } else {
            return true;
        }
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
        return Take(RVR_FWD(r), n);
    }
} inline constexpr take;

template <typename Derived>
constexpr auto RiverBase<Derived>::take(int n) & {
    return Take(self(), n);
}

template <typename Derived>
constexpr auto RiverBase<Derived>::take(int n) && {
    return Take(RVR_FWD(self()), n);
}

}

#endif
