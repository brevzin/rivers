#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>
#include <ranges>
#include <rivers/rivers.hpp>

int main() {
    namespace an = ankerl::nanobench;
    auto bench = an::Bench().minEpochIterations(50);

    std::vector<int> bunch_of_ints(1'000'000);
    std::iota(bunch_of_ints.begin(), bunch_of_ints.end(), 0);

    auto is_even = [](int x) { return x % 2 == 0; };
    auto triple = [](int x) { return 3 * x; };

    bench.run("transform_filter_handwritten",
        [&]{
            int res = 0;
            for (int i : bunch_of_ints) {
                i = triple(i);
                if (is_even(i)) {
                    res += i;
                }
            }
            an::doNotOptimizeAway(res);
        });

    bench.run("transform_filter_ranges",
        [&]{
            namespace rv = std::views;
            auto r = bunch_of_ints
                   | rv::transform(triple)
                   | rv::filter(is_even);

            int res = 0;
            for (int i : r) {
                res += i;
            }
            an::doNotOptimizeAway(res);
        });

    bench.run("transform_filter_rivers",
        [&]{
            auto r = rvr::from_cpp(bunch_of_ints)
                   .map(triple)
                   .filter(is_even);
            an::doNotOptimizeAway(r.sum());
        });
}
