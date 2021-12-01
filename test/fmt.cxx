#include "catch.hpp"

#include <fmt/format.h>
#include <rivers/rivers.hpp>

TEST_CASE("format seq", "[fmt]") {
    CHECK(fmt::format("{}", rvr::seq(5)) == "[0, 1, 2, 3, 4]");
    CHECK(fmt::format("{::#x}", rvr::seq(15, 18)) == "[0xf, 0x10, 0x11]");
    CHECK(fmt::format("{}", rvr::seq('a', 'c')) == "['a', 'b']");
    CHECK(fmt::format("{:s}", rvr::seq('a', 'c')) == "ab");
    CHECK(fmt::format("{:?s}", rvr::seq('a', 'c')) == "\"ab\"");
}

TEST_CASE("format more complex", "[fmt]") {
    auto r = rvr::seq(10)
           .filter([](int i){ return i % 2 == 0;})
           .map([](int i){ return i * i; });
    CHECK(fmt::format("{}", r) == "[0, 4, 16, 36, 64]");
}
