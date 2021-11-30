# Rivers

This is a C++20 library inspired by Java's [Streams](https://docs.oracle.com/javase/8/docs/api/java/util/stream/Stream.html). It's called Rivers solely because it allows the namespace `rvr`, which looks a lot like `rv` - which is the typical abbreviated namespace for `ranges::views`.

## Basics

While C++ Ranges are externally iterated, this library is internally iterated. This makes for a simpler model which can have better performance, although it also means it is fundamentally extremely limited in functionality. It's pretty similar to the [transrangers](https://github.com/joaquintides/transrangers), I will point out differences later.

A River is a class that provides, at a bare minimum, two pieces of functionality:

* a `reference` type, which identifies the element type
* a function template, `while_`, which takes a predicate, to loop over all the elements

For example, the following is a River over a sequence of integers (similar to Python's `range`):

```cpp
struct Ints : rvr::RiverBase<Ints> {
private:
    int from = 0;
    int to;
    int stride = 1;

public:
    constexpr Ints(int to) : to(to) { }
    constexpr Ints(int from, int to, int stride=1) : from(from), to(to), stride(stride) { }

    using reference = int;

    constexpr auto while_(rvr::Predicate<int> auto&& f) -> bool {
        while (from < to) {
            RVR_SCOPE_EXIT { ++from; };
            if (not std::invoke(f, int(from))) {
                return false;
            }
        }
        return true;
    }
};
```

This illustrates the behavior for `while_`: we have to loop over all of the elements until the predicate tells us to stop. If that happens, we return `false`. Otherwise, we return `true`.

The important thing to point out is that this operation is stateful and consuming. If a call to `r.while_(~)` for some predicate consumes the whole river (and thus returns `true`), then a subsequent call to `r.while_(~)` for _any_ predicate should immediately return `true` without doing any more work.

## Characteristics

C++ Ranges have iterator categories: they can be input, forward, bidirectional, random access, or, in C++20, contiguous. C++ Ranges can also be common or not, sized or not, const-iterable or not, borrowed or not.

Rivers have far fewer knobs. Really only one. A river can either be *resettable* or not. By default, a river is not resettable, but can opt in by providing a member function `void reset();` that resets the river to its initial state. In some cases, this is an easy operation to provide. For instance, a river from a C++ range would look [like this](include/rivers/from_cpp.hpp):

```cpp
template <std::ranges::input_range R>
struct FromCpp : RiverBase<FromCpp<R>>
{
private:
    R base;
    std::ranges::iterator_t<R> it = std::ranges::begin(base);
    std::ranges::sentinel_t<R> end = std::ranges::end(base);

public:
    using reference = std::ranges::range_reference_t<R>;
    using value_type = std::ranges::range_value_t<R>;

    constexpr FromCpp(R&& r) : base(std::move(r)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        while (it != end) {
            RVR_SCOPE_EXIT { ++it; };
            if (not std::invoke(pred, *it)) {
                return false;
            }
        }
        return true;
    }

    void reset() requires std::ranges::forward_range<R> {
        it = std::ranges::begin(base);
    }
};
```

We have to have the iterator and sentinel as members in order to satisfy the statefulness of `while_`. For forward-or-better ranges, `reset()`ing is a simple call to `begin` on the range that we have to hold onto anyway. But input ranges are single-pass, so in this case we do not provide a `reset`.

## Provided Algorithms

The library provides a few ways to generate a river:

* [seq](#seq)
* [of](#of)
* [from_cpp](#from_cpp)

Several algorithms that are available as member functions on any river (which come from inheriting from the CRTP base class `rvr::RiverBase`)

* [_](#_)
* [all](#all)
* [any](#any)
* [none](#none)
* [for_each](#for_each)
* [next](#next)
* [next_ref](#next_ref)
* [fold](#fold)
* [sum](#sum)

And several river adapters, that likewise are member functions (but require including an additional header)

* [map](#map)
* [filter](#filter)
