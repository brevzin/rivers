#ifndef RIVERS_FROM_STREAM_HPP
#define RIVERS_FROM_STREAM_HPP

#include <rivers/core.hpp>
#include <istream>

namespace rvr {

template <typename T>
    requires std::default_initializable<T>
struct FromStream : RiverBase<FromStream<T>>
{
private:
    std::istream& is;
    T value;

public:
    using reference = T&;
    constexpr FromStream(std::istream& is) : is(is) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        while (is >> value) {
            if (not std::invoke(pred, value)) {
                return false;
            }
        }
        return true;
    }
};

template <typename T>
struct from_stream_fn {
    constexpr auto operator()(std::istream& is) const {
        return FromStream<T>(is);
    }
};

template <typename T>
inline constexpr from_stream_fn<T> from_stream;

}

#endif
