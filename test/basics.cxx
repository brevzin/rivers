#include "catch.hpp"
#include <rivers/rivers.hpp>
#include <vector>
#include <sstream>

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
    CHECK(ints.any([](int i){ return i > 5; }));
    CHECK_FALSE(ints.any([](int i){ return i > 9; }));
    CHECK_FALSE(ints.none());
    CHECK(ints.none([](int i){ return i > 9; }));

    CHECK(ints.sum() == 45);
    CHECK(ints.product() == 362880);

    constexpr int s = rvr::range(100).sum();
    CHECK(s == 4950);
}

TEST_CASE("vector") {
    std::vector v = {1, 2, 3};
    auto r = rvr::from_cpp(v);
    CHECK(std::copyable<decltype(r)>);
    CHECK(rvr::multipass<decltype(r)>);
    CHECK(r.sum() == 6);

    // rvalue? takes ownership, no dangling here
    auto r2 = rvr::from_cpp(std::vector{3, 4, 5});
    CHECK(r2.product() == 60);
}

TEST_CASE("input range") {
    std::string s = "1 2 3 4 5";
    std::istringstream iss(s);
    auto r = rvr::from_cpp(std::ranges::istream_view<int>(iss));
    CHECK_FALSE(rvr::multipass<decltype(r)>);
    CHECK(r.sum() == 15);
}
