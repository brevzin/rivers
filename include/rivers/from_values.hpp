#ifndef RIVERS_FROM_VALUES_HPP
#define RIVERS_FROM_VALUES_HPP

#include <rivers/from_cpp.hpp>

////////////////////////////////////////////////////////////////////////////
// Convert specifically provided values to a River
// * from_values({1, 2, 3})
// * from_values(4, 5, 6)
// In each case, a vector is created from the arguments and that vector is
// turned into a River, via from_cpp
////////////////////////////////////////////////////////////////////////////

namespace rvr {

struct {
    template <typename T>
    constexpr auto operator()(std::initializer_list<T> values) const {
        return from_cpp(std::vector(values.begin(), values.end()));
    }

    template <typename... Ts>
    constexpr auto operator()(Ts const&... values) const {
        return from_cpp(std::vector{values...});
    }
} inline constexpr from_values;

}

#endif
