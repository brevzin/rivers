#ifndef RIVERS_SPLIT_HPP
#define RIVERS_SPLIT_HPP

#include <rivers/core.hpp>
#include <rivers/ref.hpp>

namespace rvr {

///////////////////////////////////////////////////////////////////////////
// split: takes a (RiverOf<T> r, T v) and produces a new RiverOf<RiverOf<T>>
// that is ... split... on ever instance of v
////////////////////////////////////////////////////////////////////////////

template <River R>
struct Split : RiverBase<Split<R>>
{
private:
    struct Inner : RiverBase<Inner>
    {
    private:
        friend Split;
        Split* parent;
        bool found_delim = false;

    public:
        using reference = reference_t<R>;
        explicit Inner(Split* p) : parent(p) { }

        constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool;
    };

    R base;
    value_t<R> delim;
    bool exhausted = false;
    bool partial = false;
    Inner inner;

public:
    using reference = Ref<Inner>;

    constexpr Split(R base, value_t<R> delim)
        : base(std::move(base))
        , delim(std::move(delim))
        , inner(this)
    { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        if (exhausted) {
            return true;
        }

        if (partial) {
            inner.consume();
            if (exhausted) {
                return true;
            }
            partial = false;
        }

        for (;;) {
            inner = Inner(this);
            if (std::invoke(pred, inner.ref())) {
                inner.consume();
                if (exhausted) {
                    return true;
                }
            } else {
                partial = not inner.found_delim;
                return false;
            }
        }
    }

    void reset() requires ResettableRiver<R> {
        base.reset();
        exhausted = false;
    }
};

template <River R>
constexpr auto Split<R>::Inner::while_(PredicateFor<reference> auto&& pred) -> bool {
    if (found_delim) {
        return true;
    }

    bool result = parent->base.while_([&](reference elem){
        if (elem == parent->delim) {
            found_delim = true;
            return false;
        } else {
            return std::invoke(pred, elem);
        }
    });

    if (result and not found_delim) {
        parent->exhausted = true;
    }
    return result;
}

struct {
    template <River R>
    constexpr auto operator()(R&& r, value_t<R> delim) const {
        return Split(RVR_FWD(r), std::move(delim));
    }
} inline constexpr split;

template <typename Derived>
template <typename D>
constexpr auto RiverBase<Derived>::split(value_t<D> delim) & {
    return Split(self(), std::move(delim));
}

template <typename Derived>
template <typename D>
constexpr auto RiverBase<Derived>::split(value_t<D> delim) && {
    return Split(RVR_FWD(self()), std::move(delim));
}


}

#endif
