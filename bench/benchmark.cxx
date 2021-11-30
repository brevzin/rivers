#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>
#include <ranges>

int main() {
    auto bench = ankerl::nanobench::Bench().minEpochIterations(50);

    std::vector<int> bunch_of_ints(1'000'000);
    std::iota(bunch_of_ints.begin(), bunch_of_ints.end(), 0);

    auto is_even = [](int x) { return x % 2 == 0; };
    auto triple = [](int x) { return 3 * x; };

    bench.run("transform_filter_handwritten",
        [&]{
            int res = 0;
            for (int i : bunch_of_ints) {
                if (is_even(i)) {
                    res += triple(i);
                }
            }
            ankerl::nanobench::doNotOptimizeAway(res);
        });

    bench.run("transfom_filter_ranges",
        [&]{
            namespace rv = std::views;
            auto r = bunch_of_ints
                   | rv::filter(is_even)
                   | rv::transform(triple);

            int res = 0;
            for (int i : r) {
                res += i;
            }
            ankerl::nanobench::doNotOptimizeAway(res);
        });
}
