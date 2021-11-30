#ifndef RIVERS_REF_HPP
#define RIVERS_REF_HPP

#include <rivers/core.hpp>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// ref: takes a RiverOf<T> and returns a new RiverOf<T> that is a reference
// to the original (similar to ranges::ref_view)
////////////////////////////////////////////////////////////////////////////

template <River R>
struct Ref : RiverBase<Ref<R>>
{
private:
    R* base;

public:
    using reference = reference_t<R>;

    constexpr Ref(R& base) : base(&base) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return base->while_(RVR_FWD(pred));
    }

    void reset() requires ResettableRiver<R> {
        base->reset();
    }
};

struct {
    template <River R>
    constexpr auto operator()(R& r) const {
        return Ref(r);
    }
} inline constexpr ref;

template <typename Derived>
constexpr auto RiverBase<Derived>::ref() & {
    return Ref(self());
}


}

#endif
