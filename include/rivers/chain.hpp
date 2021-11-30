#ifndef RIVERS_CHAIN_HPP
#define RIVERS_CHAIN_HPP

#include <rivers/core.hpp>

namespace rvr {

////////////////////////////////////////////////////////////////////////////
// chain: takes N RiverOf<T>'s and produces a single RiverOf<T> that consumes
// them sequentially
////////////////////////////////////////////////////////////////////////////

template <River... Rs>
    requires requires {
        typename std::common_reference_t<reference_t<Rs>...>;
    }
struct Chain : RiverBase<Chain<Rs...>>
{
private:
    std::tuple<Rs...> bases;

public:
    using reference = std::common_reference_t<reference_t<Rs>...>;

    constexpr Chain(Rs... bases) : bases(std::move(bases)...) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return std::apply([&](Rs&... rs){
            return (rs.while_(pred) and ...);
        }, bases);
    }

    void reset() requires (ResettableRiver<Rs> and ...)
    {
        std::apply([&](Rs&... rs){
            (rs.reset(), ...);
        }, bases);
    }
};

struct {
    template <River... Rs>
    constexpr auto operator()(Rs&&... rs) const {
        return Chain(RVR_FWD(rs)...);
    }
} inline constexpr chain;

template <typename Derived>
template <River... Rs>
constexpr auto RiverBase<Derived>::chain(Rs&&... rs) & {
    return Chain(self(), RVR_FWD(rs)...);
}

template <typename Derived>
template <River... Rs>
constexpr auto RiverBase<Derived>::chain(Rs&&... rs) && {
    return Chain(RVR_FWD(self()), RVR_FWD(rs)...);
}

}

#endif
