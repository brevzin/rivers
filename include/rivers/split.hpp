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
    R base;
    value_t<R> delim;
    bool exhausted = false;

public:
    struct Inner : RiverBase<Inner>
    {
        Split* parent;
        bool found_delim = false;

        using reference = reference_t<R>;

        constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
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
    };

    using reference = Ref<Inner>;

    constexpr Split(R base, value_t<R> delim) : base(std::move(base)), delim(std::move(delim)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        if (exhausted) {
            return true;
        }

        for (;;) {
            auto inner = Inner{{}, this};
            if (std::invoke(pred, inner.ref())) {
                inner.consume();
                if (exhausted) {
                    return true;
                }
            } else {
                inner.consume();
                return false;
            }
        }
    }

    void reset() requires ResettableRiver<R> {
        base.reset();
        exhausted = false;
    }
};

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
