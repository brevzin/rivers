#ifndef RIVERS_CORE_HPP
#define RIVERS_CORE_HPP

#include <concepts>
#include <ranges>
#include <rivers/optional.hpp>

#define RVR_FWD(x) static_cast<decltype(x)&&>(x)

namespace rvr {
    // A River's reference type is R::reference. This must be provided
    template <typename R>
    using reference_t = typename std::remove_cvref_t<R>::reference;

    ////////////////////////////////////////////////////////////////////////////
    // A river's value_type is more complicated.
    // If R::value_type is not simply remove_cvref_t<R::reference>, then that.
    // Otherwise, value_type_from_ref_t<rR::reference>, could simply be
    // remove_cvref_t but also recognizes pair and tuple - that tuple<T&>'s
    // value type should be tuple<T>, not simply tuple<T&>
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
        // only use R::value_type if it's not remove_cvref_t<reference>
        // (i.e. in the case that it's actually meaningful)
        // Otherwise, value_type_from_ref will do the remove_cvref_t part itself
        requires requires {
            typename R::value_type;
            requires not std::same_as<typename R::value_type,
                                      std::remove_cvref_t<reference_t<R>>>;
        }
    struct value_type_for<R> {
        using type = typename R::value_type;
    };

    template <typename R>
    using value_t = typename value_type_for<std::remove_cvref_t<R>>::type;
    ////////////////////////////////////////////////////////////////////////////


    template <typename R>
    concept River = requires (R r) {
        typename reference_t<R>;
        { r.while_(std::declval<bool(*)(reference_t<R>)>()) } -> std::same_as<bool>;
    };

    // std::predicate requires regular_invocable but we don't need that
    // also this concept checks that F is invocable as an lvalue, the intent
    // the intent is to allow for taking PredicateFor<reference> auto&& as a
    // parameter and not forwarding it
    template <typename F, typename R>
    concept PredicateFor = std::invocable<F&, R>
                        && std::convertible_to<std::invoke_result_t<F&, R>, bool>;


    // A River is reset-able if it has a member reset().
    // reset() returns the River to the beginning (i.e. its source?)
    // Such a function should only be provided if it's cheap to do so - or at
    // least cheaper than stashing a copy of the original River before any
    // mutating operations and then restoring it
    template <typename R>
    concept ResettableRiver = River<R> && requires (R r) {
            r.reset();
        };


    // simple scope guard implementation - since we're only ever capturing
    // by reference, construction can't throw, so can skip a lot of steps
    namespace detail {
        enum class scope_exit_tag { };

        template <typename F>
        struct ScopeExit {
            F func;
            constexpr ~ScopeExit() noexcept { func(); }
        };


        template <typename F>
        constexpr auto operator+(scope_exit_tag, F f) {
            return ScopeExit<F>{.func=RVR_FWD(f)};
        }
    }

    #define RVR_CONCAT2(x, y) x ## y
    #define RVR_CONCAT(x, y) RVR_CONCAT2(x, y)
    #define RVR_UNIQUE_NAME(prefix) RVR_CONCAT(prefix, __LINE__)
    #define RVR_SCOPE_EXIT                           \
        auto RVR_UNIQUE_NAME(RVR_SCOPE_EXIT_GUARD) = \
        ::rvr::detail::scope_exit_tag{} + [&]() noexcept -> void

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
        // all(pred)
        // * Returns: (... and pred(elems))
        template <typename Pred = std::identity>
            requires std::predicate<Pred&, reference_t<Derived>>
        constexpr auto all(Pred pred = {}) -> bool
        {
            return self().while_([&](reference_t<Derived> e) -> bool {
                return std::invoke(pred, e);
            });
        }

        // any()
        // * Returns (... or elems)
        // any(pred)
        // * Returns: (... or pred(elems))
        template <typename Pred = std::identity>
            requires std::predicate<Pred&, reference_t<Derived>>
        constexpr auto any(Pred pred = {}) -> bool
        {
            return not self().all(std::not_fn(pred));
        }

        // none()
        // * Returns: not any()
        // none(pred)
        // * Returns: not any(pred)
        template <typename Pred = std::identity>
            requires std::predicate<Pred&, reference_t<Derived>>
        constexpr auto none(Pred pred = {}) -> bool
        {
            return not self().any(pred);
        }

        // for_each(op)
        // Equivalent to (op(elem), ...);
        template <typename F> requires std::invocable<F&, reference_t<Derived>>
        constexpr void for_each(F&& f) {
            self().while_([&](reference_t<Derived> e){
                std::invoke(f, e);
                return true;
            });
        }

        // next()
        // * Return the next value from this river, or nullopt if none such
        // next_ref()
        // * Return the next reference from this river, or nullopt if none such
        template <typename D=Derived>
        constexpr auto next() -> tl::optional<value_t<D>> {
            tl::optional<value_t<D>> result;
            self().while_([&](reference_t<D> elem){
                result.emplace(RVR_FWD(elem));
                return false;
            });
            return result;
        }

        template <typename D=Derived>
        constexpr auto next_ref() -> tl::optional<reference_t<D>> {
            tl::optional<reference_t<D>> result;
            self().while_([&](reference_t<D> elem){
                result.emplace(RVR_FWD(elem));
                return false;
            });
            return result;
        }

        // fold(init, op)
        // a left-fold that performs op(op(op(init, e0), e1), ...
        template <typename Z, typename F>
            requires requires (F op, Z init, reference_t<Derived> elem) {
                { op(std::move(init), RVR_FWD(elem)) } -> std::convertible_to<Z>;
            }
        constexpr auto fold(Z init, F op) -> Z
        {
            for_each([&](reference_t<Derived> e){
                init = op(std::move(init), RVR_FWD(e));
            });
            return init;
        }

        // sum(init = value())
        // * Returns (init + ... + elems)
        template <typename D=Derived>
        constexpr auto sum(value_t<D> init = {}) -> value_t<D>
        {
            return fold(RVR_FWD(init), std::plus());
        }

        // product(init = value(1))
        // * Returns (init * ... * elems)
        template <typename D=Derived>
        constexpr auto product(value_t<D> init = value_t<D>(1)) -> value_t<D>
        {
            return fold(RVR_FWD(init), std::multiplies());
        }

        // map(f): requires map.hpp
        template <typename F> constexpr auto map(F&& f) &;
        template <typename F> constexpr auto map(F&& f) &&;

        // filter(p): requires filter.hpp
        template <typename P = std::identity> constexpr auto filter(P&& = {}) &;
        template <typename P = std::identity> constexpr auto filter(P&& = {}) &&;

        // chain(p...): requires chain.hpp
        template <River... R> constexpr auto chain(R&&...) &;
        template <River... R> constexpr auto chain(R&&...) &&;
    };
}

#endif
