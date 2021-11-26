# Rivers

This is a C++20 library inspired by Java's Streams. It's called Rivers solely because it allows the namespace `rvr`, which looks a lot like `rv` - which is the typical abbreviated namespace for `ranges::views`.

## Basics

While C++ Ranges are externally iterated, this library is internally iterated. This makes for a simpler model which can have better performance, although it also means it is fundamentally extremely limited in functionality.

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
        for (int i = from; i < to; i += stride) {
            if (not std::invoke(f, int(i))) {
                return false;
            }
        }
        return true;
    }
};
```

This illustrates the behavior for `while_`: we have to loop over all of the elements until the predicate tells us to stop. If that happens, we return `false`. Otherwise, we return `true`.

That's it.

## Characteristics

C++ Ranges have iterator categories: they can be input, forward, bidirectional, random access, or, in C++20, contiguous. C++ Ranges can also be common or not, sized or not, const-iterable or not, borrowed or not.

Rivers have far fewer knobs. Really only one. A river can either be *multipass* or not. By default, a river is not multipass, but can opt into being so by providing a `static constexpr bool multipass = true;` data member. Multipass rivers can have their `while_` member invoked multiple times. Single-pass rivers cannot.

For a multi-pass river, each invocation of `while_` starts from the first element, regardless of when (if ever) the provided predicate returned `false`. In this sense, a river has no state or memory. As such, while it is easy to convert a C++ Range (or iterator pair) into a River, it is not feasible to convert a River into a C++ Range (or at least, it would be exceedingly expensive to do so, so it is not provided).

