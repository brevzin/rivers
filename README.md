- [Rivers](#rivers)
  - [Basics](#basics)
  - [Characteristics](#characteristics)
  - [Formatting](#formatting)
  - [Single Header](#single-header)
  - [Generators](#generators)
    - [seq](#seq)
    - [of](#of)
    - [from](#from)
  - [Extension](#extension)
  - [Terminal Algorithms](#terminal-algorithms)
    - [all](#all)
    - [any](#any)
    - [none](#none)
    - [for_each](#for_each)
    - [next](#next)
    - [next_ref](#next_ref)
    - [fold](#fold)
    - [sum](#sum)
    - [product](#product)
    - [count](#count)
    - [consume](#consume)
    - [collect](#collect)
    - [into_vec](#into_vec)
    - [into_str](#into_str)
  - [River Adapters](#river-adapters)
    - [ref](#ref)
    - [map](#map)
    - [filter](#filter)
    - [chain](#chain)
    - [take](#take)
    - [drop](#drop)
    - [split](#split)

# Rivers

[![example](https://github.com/BRevzin/rivers/actions/workflows/ci.yml/badge.svg)](https://github.com/BRevzin/rivers/actions/workflows/ci.yml)

This is a C++20 library inspired by Java's [Streams](https://docs.oracle.com/javase/8/docs/api/java/util/stream/Stream.html). It's called Rivers solely because it allows the namespace `rvr`, which looks a lot like `rv` - which is the typical abbreviated namespace for `ranges::views`.

## Basics

While C++ Ranges are externally iterated, this library is internally iterated. This makes for a simpler model which can have better performance, although it also means it is fundamentally extremely limited in functionality. It's pretty similar to the [transrangers](https://github.com/joaquintides/transrangers), which also contains a good description of what the model is and why it could be beneficial. The primary difference is that in transrangers, the consuming predicate receives a _cursor_ that dereferences into an element whereas in this library, the consuming predicate receives the element itself - this is to avoid situations like a `map` followed by a `filter` potentially invoking the `map` operation multiple times.

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

## Formatting

A formatter is provided for rivers under the header `rivers/format.hpp`. It presumes that `<fmt/format.hpp>` can be found as an include. Otherwise, it does nothing. The examples for the algorithms below will all use formatting to demonstrate the functionality. Formatting support is based on [P2286](https://wg21.link/p2286).

## Single Header

A single header version can be found [here](single_include/rivers/single_header.hpp).

## Generators

### seq

There are two overloads of `seq`:

* `seq(from, to)` produces a river that iterates from `from` up to, but not including, `to`.
* `seq(to)` produces a river that iterates from `I(0)` up to, but not including, `to`, where `I` is the type of `to`.

Similarly to `views::iota`, this river generator can accept any type that is `weakly_incrementable` and `equality_comparable`. Differently from `views::iota`, and more like Python's `range`, `seq(5)` produces the range `[0, 5)` rather than the range `[5, inf)`.

```cpp
fmt::print("{}\n", rvr::seq(1, 5)); // [1, 2, 3, 4]
fmt::print("{}\n", rvr::seq(3));    // [0, 1, 2]
```

### of

`of` is used to construct a river with the specified elements.

There are three overloads of `of`:

* `of({x, y, z})` (taking an `initializer_list<T>`) produces a river containing those elements (internally constructing a `vector` to hold them)
* `of(x, y, z)` does the same
* `of(x)` is a special case for just the single element

```cpp
fmt::print("{}\n", rvr::of(1, 9, 16)); // [1, 9, 16]
```

### from

`from` is used to turn a C++ range into a river. `from(e)` does one of:

* copies/moves `e`, if `e` is a `view`
* constructs a `ref_view(e)` if `e` is an lvalue, and constructs a river out of that
* moves `e` in, if `e` is a non-`view` range

This follows the [P2415](https://wg21.link/p2415) design for `views::all` in C++20 ranges.

## Extension

The library is written so that all the river adapters and terminal algorithms can be invoked with `.` notation. That is, `r.map(f).filter(g).any()` rather than the C++20 equivalent of `ranges::any_of(r | views::transform(f) | views::filter(g), std::identity())`. That's very convenient if you only use algorithms provided by the library, but not so convenient if you want to... do something else.

To that end, `RiverBase` provides a `_` member function such that `r._(f, args...)` evaluates as `f(r, args...)`. This is a heavily simplified version of the `|` support that range-v3/C++20 Ranges provide, and also doesn't require any notion of "partial call".

The library provides function objects for each algorithm as well. So these are all equivalent:

1. `r.map(f)`
2. `rvr::map(r, f)`
3. `r._(rvr::map, f)`

I don't imagine anybody would prefer (3) to (1) or (2) for `map` specifically, but the existence of the syntax allows for a more convenient flow with user-defined river adapters.

## Terminal Algorithms

These are the point of Streams - is to run a terminal algorithm more efficiently than you could with either of the C++, D, or Rust iteration models.


### all

TODO

### any

TODO

### none

TODO

### for_each

TODO

### next

TODO

### next_ref

TODO

### fold

TODO

### sum

TODO

### product

TODO

### count

TODO

### consume

TODO

### collect

TODO

### into_vec

TODO

### into_str

TODO

## River Adapters

These are algorithms that take one River and produce another River.


### ref
### map
### filter
### chain
### take
### drop
### split
