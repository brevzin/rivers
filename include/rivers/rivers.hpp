#ifndef RIVERS_RIVERS_HPP
#define RIVERS_RIVERS_HPP

#include <concepts>

#define RVR_FWD(x) static_cast<decltype(x)&&>(x)

namespace rvr {
    template <typename R>
    using reference_t = typename R::reference;

    template <typename T>
    struct value_type_from_ref_t {
        using type = std::remove_cvref_t<T>;
    };

    template <typename T>
    using value_type_from_ref = typename value_type_from_ref_t<T>::type;

    template <typename T, typename U>
    struct value_type_from_ref_t<std::pair<T, U>> {
        using type = std::pair<value_type_from_ref<T>, value_type_from_ref<U>>;
    };

    template <typename... Ts>
    struct value_type_from_ref_t<std::tuple<Ts...>> {
        using type = std::tuple<value_type_from_ref<Ts>...>;
    };

    template <typename R>
    struct value_type_for {
        // if there's no value_type
        using type = value_type_from_ref<reference_t<R>>;
    };

    template <typename R>
        requires requires { typename R::value_type; }
    struct value_type_for<R> {
        using type = typename R::value_type;
    };

    template <typename R>
    using value_type_t = typename value_type_for<R>::type;

    template <typename R>
    inline constexpr bool multipass = false;

    template <typename R> requires requires { R::multipass; }
    inline constexpr bool multipass<R> = R::multipass;

    template <typename R>
    concept River = requires (R r) {
        typename reference_t<R>;
        { r.for_each(std::declval<bool(*)(reference_t<R>)>()) } -> std::same_as<bool>;
    };

    // std::predicate requires regular_invocable but we don't need that
    // also this concept checks that F is invocable as an lvalue, the intent
    // the intent is to allow for taking PredicateFor<reference> auto&& as a
    // parameter and not forwarding it
    template <typename F, typename R>
    concept PredicateFor = std::invocable<F&, R>
                        && std::convertible_to<std::invoke_result_t<F&, R>, bool>;


    template <typename Derived>
    struct RiverBase {
    private:
        constexpr auto self() -> Derived& { return *(Derived*)this; }

    public:
        // rather than using | as Ranges do, as in
        //      r | views::transform(f)
        // here I'm instead doing
        //      r._(rvr::transform, f)
        // which simply takes less work to be able to put together.
        template <typename F, typename... Args>
        constexpr auto _(F&& f, Args&&... args) & {
            return RVR_FWD(f)(self(), RVR_FWD(args)...);
        }

        template <typename F, typename... Args>
            requires std::invocable<F, Derived, Args...>
        constexpr auto _(F&& f, Args&&... args) && {
            return RVR_FWD(f)(std::move(self()), RVR_FWD(args)...);
        }

        ///////////////////////////////////////////////////////////////////
        // and a bunch of common algorithms
        ///////////////////////////////////////////////////////////////////

        // all()
        // * Returns: (... and elems)
        constexpr auto all() -> bool
            requires std::convertible_to<reference_t<Derived>, bool>
        {
            return self().for_each([](bool b){ return b; });
        }

        // all(p)
        // * Returns: (... and pred(elems))
        template <typename Pred>
            requires std::predicate<Pred&, reference_t<Derived>>
        constexpr auto all(Pred pred) -> bool
        {
            return self().for_each([&](reference_t<Derived> e) -> bool {
                return std::invoke(pred, e);
            });
        }

        // any()
        // * Returns (... or elems)
        constexpr auto any() -> bool
            requires std::convertible_to<reference_t<Derived>, bool>
        {
            return not self().for_each([](bool b){ return not b; });
        }

        // none()
        // * Returns: not all()
        constexpr auto none() -> bool
            requires std::convertible_to<reference_t<Derived>, bool>
        {
            return not all();
        }

        // fold(init, op)
        // a left-fold that performs op(op(op(init, e0), e1), ...
        template <typename Z, typename F>
            requires requires (F op, Z init, reference_t<Derived> elem) {
                { op(std::move(init), RVR_FWD(elem)) } -> std::convertible_to<Z>;
            }
        constexpr auto fold(Z init, F op) -> Z
        {
            self().for_each([&](reference_t<Derived> e){
                init = op(std::move(init), RVR_FWD(e));
                return true;
            });
            return init;
        }

        // sum()
        // * Returns (value() + ... + elems)
        template <typename D=Derived>
        constexpr auto sum() -> value_type_t<D>
        {
            return fold(value_type_t<D>(), std::plus());
        }

        // product()
        // * Returns (value(1) * ... * elems)
        template <typename D=Derived>
        constexpr auto product() -> value_type_t<D>
        {
            return fold(value_type_t<D>(1), std::multiplies());
        }

    };

    // Python-style range.
    // range(3, 5) includes the elements [3, 4]
    // range(5) includes the elements [0, 1, 2, 3, 4]
    // Always multipass
    // Like std::views::iota, except that range(e) are the elements in [E(), e)
    // rather than the  being the elements in [e, inf)
    template <std::weakly_incrementable I>
        requires std::equality_comparable<I>
    struct Range : RiverBase<Range<I>>
    {
    private:
        I from = I();
        I to;

    public:
        constexpr Range(I from, I to) : from(from), to(to) { }
        constexpr Range(I to) requires std::default_initializable<I> : to(to) { }

        static constexpr bool multipass = true;
        using reference = I;

        constexpr auto for_each(PredicateFor<reference> auto&& pred) -> bool {
            for (I i = from; i != to; ++i) {
                if (not std::invoke(pred, I(i))) {
                    return false;
                }
            }
            return true;
        }
    };

    struct {
        template <std::weakly_incrementable I> requires std::equality_comparable<I>
        constexpr auto operator()(I from, I to) const {
            return Range(std::move(from), std::move(to));
        }

        template <std::weakly_incrementable I>
            requires std::equality_comparable<I>
                  && std::default_initializable<I>
        constexpr auto operator()(I to) const {
            return Range(std::move(to));
        }
    } inline constexpr range;
}


#endif
