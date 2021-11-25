#include "catch.hpp"
#include <rivers/rivers.hpp>

struct S { };
static_assert(not rvr::River<S>);

struct Ints {
    using reference = int;

    auto for_each(rvr::PredicateFor<int> auto&& f) -> bool;
};
static_assert(rvr::River<Ints>);
static_assert(std::same_as<rvr::reference_t<Ints>, int>);
static_assert(not rvr::multipass<Ints>);

TEST_CASE("range") {
    auto ints = rvr::range(1, 10);
    CHECK(ints.all());
    CHECK_FALSE(ints.all([](int i){ return i % 2 == 0; }));
    CHECK(ints.any());
    CHECK_FALSE(ints.none());

    CHECK(ints.sum() == 45);
    CHECK(ints.product() == 362880);

    constexpr int s = rvr::range(100).sum();
    CHECK(s == 4950);
}
