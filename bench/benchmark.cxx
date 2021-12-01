#define ANKERL_NANOBENCH_IMPLEMENT
#include <ranges>
#include <rivers/rivers.hpp>
#include "nanobench.h"
#include "flow.hpp"
#include <range/v3/view/concat.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/numeric/accumulate.hpp>

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

    bench.run("transform_filter_rivers",
        [&]{
            auto r = rvr::from(bunch_of_ints)
                   .map(triple)
                   .filter(is_even);
            an::doNotOptimizeAway(r.sum());
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

    bench.run("transform_filter_range-v3",
        [&]{
            namespace rv = ranges::views;
            auto r = bunch_of_ints
                   | rv::transform(triple)
                   | rv::filter(is_even);
            an::doNotOptimizeAway(ranges::accumulate(r, 0));
        });

    bench.run("transform_filter_flow",
        [&]{
            auto r = flow::from(bunch_of_ints)
                   .map(triple)
                   .filter(is_even);
            an::doNotOptimizeAway(r.sum());
        });

    auto moar_ints = bunch_of_ints;
    std::reverse(moar_ints.begin(), moar_ints.end());

    bench.run("concat_handwritten", [&]{
        int res = 0;
        for (int i : bunch_of_ints) {
            res += i;
        }
        for (int i : moar_ints) {
            res += i;
        }
        an::doNotOptimizeAway(res);
    });

    bench.run("concat_rivers", [&]{
        auto r = rvr::from(bunch_of_ints)
               .chain(rvr::from(moar_ints));
        an::doNotOptimizeAway(r.sum());
    });

    bench.run("concat_range-v3", [&]{
        namespace rv = ranges::views;
        auto r = rv::concat(bunch_of_ints, moar_ints);
        an::doNotOptimizeAway(ranges::accumulate(r, 0));
    });

    bench.run("concat_flow", [&]{
        auto r = flow::chain(flow::from(bunch_of_ints), flow::from(moar_ints));
        an::doNotOptimizeAway(r.sum());
    });

    bench.run("concat_take_transform_filter_handwritten", [&]{
        int res = 0;
        int take = 1'500'000;
        for (int i : bunch_of_ints) {
            if (take == 0) {
                break;
            }
            --take;
            i = triple(i);
            if (is_even(i)) {
                res += i;
            }
        }
        for (int i : moar_ints) {
            if (take == 0) {
                break;
            }
            --take;
            i = triple(i);
            if (is_even(i)) {
                res += i;
            }
        }
        an::doNotOptimizeAway(res);
    });

    bench.run("concat_take_transform_filter_rivers", [&]{
        auto r = rvr::from(bunch_of_ints)
               .chain(rvr::from(moar_ints))
               .take(1'500'000)
               .map(triple)
               .filter(is_even);
        an::doNotOptimizeAway(r.sum());
    });

    bench.run("concat_take_transform_filter_range-v3", [&]{
        namespace rv = ranges::views;
        auto r = rv::concat(bunch_of_ints, moar_ints)
               | rv::take(1'500'000)
               | rv::transform(triple)
               | rv::filter(is_even);
        an::doNotOptimizeAway(ranges::accumulate(r, 0));
    });

    bench.run("concat_take_transform_filter_flow", [&]{
        auto r = flow::chain(flow::from(bunch_of_ints), flow::from(moar_ints))
               .take(1'500'000)
               .map(triple)
               .filter(is_even);
        an::doNotOptimizeAway(r.sum());
    });
}
