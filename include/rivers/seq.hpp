#ifndef RIVERS_SEQ_HPP
#define RIVERS_SEQ_HPP

#include <rivers/core.hpp>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// Python-style range.
// seq(3, 5) includes the elements [3, 4]
// seq(5) includes the elements [0, 1, 2, 3, 4]
// Always multipass
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

public:
    static constexpr bool multipass = true;
    using reference = I;

    constexpr Seq(I from, I to) : from(from), to(to) { }
    constexpr Seq(I to) requires std::default_initializable<I> : to(to) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        for (I i = from; i != to; ++i) {
            if (not std::invoke(pred, I(i))) {
                return false;
            }
        }
        return true;
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
