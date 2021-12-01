#ifndef RIVERS_COLLECT_HPP
#define RIVERS_COLLECT_HPP

#include <rivers/core.hpp>
#include <rivers/tag_invoke.hpp>
#include <vector>
#include <string>

namespace rvr {

template <typename T>
struct collect_fn {
    template <River R>
        requires std::ranges::input_range<T>
    friend constexpr auto tag_invoke(collect_fn<T>, R&& r) -> T {
        T output;
        r.for_each([&](auto&& elem){
            output.push_back(RVR_FWD(elem));
        });
        return output;
    }

    template <River R>
        requires tag_invocable<collect_fn, R>
    constexpr auto operator()(R&& r) const {
        return rvr::tag_invoke(*this, RVR_FWD(r));
    }
};

template <typename T>
inline constexpr collect_fn<T> collect;


template <typename Derived>
template <typename T>
constexpr auto RiverBase<Derived>::collect() {
    return rvr::collect<T>(self());
}

template <typename Derived>
template <template <typename...> class Z>
constexpr auto RiverBase<Derived>::collect() {
    struct iterator {
        using value_type = value_t<Derived>;
        using reference = reference_t<Derived>;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        auto operator*() const -> reference;
        auto operator++() -> iterator&;
        auto operator++(int) -> iterator;
        auto operator==(iterator const&) const -> bool;
    };
    static_assert(std::input_iterator<iterator>);
    using T = decltype(Z(std::declval<iterator>(), std::declval<iterator>()));
    return collect<T>();
}

template <typename Derived>
constexpr auto RiverBase<Derived>::into_vec() {
    return collect<std::vector>();
}

template <typename Derived>
constexpr auto RiverBase<Derived>::into_str() {
    return collect<std::string>();
}


}

#endif
