#ifndef RIVERS_OF_HPP
#define RIVERS_OF_HPP

#include <rivers/core.hpp>
#include <rivers/from.hpp>

////////////////////////////////////////////////////////////////////////////
// Convert specifically provided values to a River
// * from_values({1, 2, 3})
// * from_values(4, 5, 6)
// In each case, a vector is created from the arguments and that vector is
// turned into a River, via from_cpp
////////////////////////////////////////////////////////////////////////////

namespace rvr {

template <typename T>
struct Single : RiverBase<Single<T>> {
private:
    T value;
    bool consumed = false;

public:
    using reference = T&;

    template <typename U>
    explicit constexpr Single(U&& value) : value(RVR_FWD(value)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        if (not consumed) {
            RVR_SCOPE_EXIT { consumed = true; };
            return std::invoke(pred, value);
        } else {
            return false;
        }
    }

    void reset() {
        consumed = false;
    }
};

struct {
    template <typename T>
    constexpr auto operator()(std::initializer_list<T> values) const {
        return from(std::vector(values.begin(), values.end()));
    }

    template <typename... Ts>
    constexpr auto operator()(Ts&&... values) const {
        return from(std::vector{values...});
    }

    // special case a single element
    template <typename T>
    constexpr auto operator()(T&& value) const {
        return Single<std::decay_t<T>>(RVR_FWD(value));
    }
} inline constexpr of;

}

#endif
