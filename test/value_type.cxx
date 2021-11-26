#include "catch.hpp"
#include <rivers/rivers.hpp>

namespace {
    template <typename R, typename V>
    struct Test {
        using value_type = V;
        using reference = R;
    };

    template <typename R>
    struct Test<R, void> {
        using reference = R;
    };
}

static_assert(std::same_as<
    rvr::value_type_t<Test<int&, int>>, int>);
static_assert(std::same_as<
    rvr::value_type_t<Test<int const&, int>>, int>);
static_assert(std::same_as<
    rvr::value_type_t<Test<int&, void>>, int>);
static_assert(std::same_as<
    rvr::value_type_t<Test<int const&, void>>, int>);

static_assert(std::same_as<
    rvr::value_type_t<Test<std::tuple<int&>, std::tuple<int&>>>, std::tuple<int>>);
