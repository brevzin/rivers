#include "catch.hpp"

#include <string>
#include <vector>
#include <rivers/rivers.hpp>
#include "test_utils.hpp"

TEST_CASE("count words", "[split]") {
    std::string s = "A bunch of words";
    auto r = rvr::from_cpp(s).split(' ');

    using R = decltype(r);
    STATIC_REQUIRE(rvr::River<R>);
    STATIC_REQUIRE(rvr::River<rvr::reference_t<R>>);
    STATIC_REQUIRE(std::same_as<rvr::reference_t<rvr::reference_t<R>>, char&>);

    CHECK(r.count() == 4);
}

TEST_CASE("count letters with map", "[split]") {
    std::string s = "A bunch of words";
    auto r = rvr::from_cpp(s).split(' ').map([](auto&& word){ return word.count(); });
    using R = decltype(r);
    STATIC_REQUIRE(rvr::River<R>);
    STATIC_REQUIRE(std::same_as<rvr::reference_t<R>, int>);
    CHECK(r.sum() == 13);
    CHECK(r.sum() == 0);

    r.reset();
    CHECK(r.next() == Some(1));
    CHECK(r.next() == Some(5));
    CHECK(r.next() == Some(2));
    CHECK(r.next() == Some(5));
    CHECK_FALSE(r.next());
}

TEST_CASE("first letter of each word", "[split]") {
    std::string s = "A bunch of words";
    auto r = rvr::from_cpp(s)
           .split(' ')
           .map([](auto&& word){ return *word.next(); });

    CHECK(r.next() == Some('A'));
    CHECK(r.next() == Some('b'));
    CHECK(r.next() == Some('o'));
    CHECK(r.next() == Some('w'));
    CHECK_FALSE(r.next());
}

TEST_CASE("multiple delimiters", "[split]") {
    std::string s = "two  words ";
    auto r = rvr::from_cpp(s).split(' ');
    CHECK(r.count() == 4);
    r.reset();

    auto counts = std::move(r).map([](auto&& word){ return word.count(); });
    CHECK(counts.next() == Some(3));
    CHECK(counts.next() == Some(0));
    CHECK(counts.next() == Some(5));
    CHECK(counts.next() == Some(0));
    CHECK_FALSE(counts.next());
}

TEST_CASE("collect each word", "[split]") {
    using namespace std::literals;
    std::string s = "A bunch of words";
    {
        auto r = rvr::from_cpp(s).split(' ').map(rvr::collect<std::string>);
        CHECK(r.next() == Some("A"s));
        CHECK(r.next() == Some("bunch"s));
        CHECK(r.next() == Some("of"s));
        CHECK(r.next() == Some("words"s));
        CHECK_FALSE(r.next());
    }

    {
        auto r = rvr::from_cpp(s).split(' ').map([](auto&& word){
            return word.into_vec();
        });
        CHECK(r.next() == Some(std::vector{'A'}));
        CHECK(r.next() == Some(std::vector{'b', 'u', 'n', 'c', 'h'}));
        CHECK(r.next() == Some(std::vector{'o', 'f'}));
        CHECK(r.next() == Some(std::vector{'w', 'o', 'r', 'd', 's'}));
        CHECK_FALSE(r.next());
    }
}
