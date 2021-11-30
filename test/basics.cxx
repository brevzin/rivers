#include "catch.hpp"

#include <vector>
#include <sstream>
#include <rivers/rivers.hpp>

struct S { };
static_assert(not rvr::River<S>);

struct Ints {
    using reference = int;

    auto while_(rvr::PredicateFor<int> auto&& f) -> bool;
};
static_assert(rvr::River<Ints>);
static_assert(std::same_as<rvr::reference_t<Ints>, int>);
static_assert(rvr::River<Ints&>);
static_assert(not rvr::River<Ints const&>);

namespace Catch {
    template <typename T>
    struct StringMaker<tl::optional<T>> {
        static auto convert(tl::optional<T> const& value) -> std::string {
            ReusableStringStream rs;
            if (value) {
                rs << "Some(" << Detail::stringify(*value) << ')';
            } else {
                rs << "None";
            }
            return rs.str();
        }
    };
}

template <typename T>
auto Some(T&& t) -> tl::optional<std::remove_cvref_t<T>> {
    return RVR_FWD(t);
}

TEST_CASE("seq") {
    auto ints = rvr::seq(1, 10);

    CHECK(ints.next() == Some(1));
    CHECK(ints.next() == Some(2));
    CHECK(ints.sum() == 42);
    CHECK_FALSE(ints.next());

    CHECK(rvr::ResettableRiver<decltype(ints)>);
    ints.reset();
    CHECK(ints.next() == Some(1));

    CHECK_FALSE(rvr::seq(10).all());
    CHECK(rvr::seq(1, 10).all());
    CHECK_FALSE(rvr::seq(10).all([](int i){ return i % 2 == 0; }));
    CHECK(rvr::seq(10).any());
    CHECK(rvr::seq(10).any([](int i){ return i > 5; }));
    CHECK_FALSE(rvr::seq(10).any([](int i){ return i > 9; }));
    CHECK_FALSE(rvr::seq(10).none());
    CHECK(rvr::seq(10).none([](int i){ return i > 9; }));

    CHECK(rvr::seq(10).sum() == 45);
    CHECK(rvr::seq(10).product() == 0);
    CHECK(rvr::seq(1, 10).product() == 362880);

    constexpr int s = rvr::seq(100).sum();
    CHECK(s == 4950);
}

TEST_CASE("vector") {
    std::vector v = {1, 2, 3};
    auto r = rvr::from_cpp(v);
    CHECK(std::copyable<decltype(r)>);
    CHECK(rvr::ResettableRiver<decltype(r)>);
    CHECK(r.sum() == 6);
    CHECK(r.sum() == 0);
    r.reset();
    CHECK(r.sum() == 6);

    r = rvr::from_cpp(v);
    auto front = r.next_ref();
    REQUIRE(front);
    CHECK(&*front == v.data());

    // rvalue? takes ownership, no dangling here
    auto r2 = rvr::from_cpp(std::vector{3, 4, 5});
    CHECK(r2.product() == 60);
}

TEST_CASE("input range") {
    std::string s = "1 2 3 4 5";
    std::istringstream iss(s);
    auto r = rvr::from_cpp(std::ranges::istream_view<int>(iss));
    CHECK(r.sum() == 15);
    CHECK_FALSE(rvr::ResettableRiver<decltype(r)>);
}

TEST_CASE("map") {
    auto squares = rvr::seq(1, 5)._(rvr::map, [](int i){ return i*i; });
    CHECK(squares.sum() == 30);
    CHECK(rvr::ResettableRiver<decltype(squares)>);
    squares.reset();
    CHECK(squares.next() == Some(1));
    CHECK(squares.next() == Some(4));
    CHECK(squares.next() == Some(9));
    CHECK(squares.next() == Some(16));
    CHECK_FALSE(squares.next());

    auto squares2 = rvr::seq(1, 5).map([](int i){ return i*i; });
    CHECK(squares2.product() == 576);
}

TEST_CASE("of") {
    using namespace std::literals;
    CHECK(rvr::of("hello"s, "world"s).none(&std::string::empty));

    auto one = rvr::of(1);
    CHECK(one.next() == Some(1));
    CHECK_FALSE(one.next());
    one.reset();
    CHECK(one.next() == Some(1));
}

TEST_CASE("filter") {
    auto evens = rvr::seq(100).filter([](int i){ return i % 2 == 0; });

    CHECK(evens.next() == Some(0));
    CHECK(evens.next() == Some(2));
    CHECK(evens.next() == Some(4));
    evens.reset();
    CHECK(evens.sum() == 2450);
}
