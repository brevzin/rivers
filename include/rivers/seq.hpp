#ifndef RIVERS_SEQ_HPP
#define RIVERS_SEQ_HPP

#include <rivers/core.hpp>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// Python-style range.
// seq(3, 5) includes the elements [3, 4]
// seq(5) includes the elements [0, 1, 2, 3, 4]
// Like std::views::iota, except that seq(e) are the elements in [E(), e)
// rather than the  being the elements in [e, inf)
////////////////////////////////////////////////////////////////////////////
template <std::weakly_incrementable I>
    requires std::equality_comparable<I>
struct Seq : RiverBase<Seq<I>>
{
private:
    I from = I();
    I to;
    I orig = from;

public:
    using reference = I;

    constexpr Seq(I from, I to) : from(from), to(to) { }
    constexpr Seq(I to) requires std::default_initializable<I> : to(to) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        while (from != to) {
            RVR_SCOPE_EXIT { ++from; };
            if (not std::invoke(pred, I(from))) {
                return false;
            }
        }
        return true;
    }

    void reset() {
        from = orig;
    }
};

struct {
    template <std::weakly_incrementable I> requires std::equality_comparable<I>
    constexpr auto operator()(I from, I to) const {
        return Seq(std::move(from), std::move(to));
    }

    template <std::weakly_incrementable I>
        requires std::equality_comparable<I>
                && std::default_initializable<I>
    constexpr auto operator()(I to) const {
        return Seq(std::move(to));
    }
} inline constexpr seq;

}

#endif
