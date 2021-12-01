#ifndef RIVERS_RIVERS_HPP
#define RIVERS_RIVERS_HPP

#ifndef RIVERS_CORE_HPP
#define RIVERS_CORE_HPP

#include <concepts>
#include <ranges>

///
// optional - An implementation of std::optional with extensions
// Written in 2017 by Sy Brand (tartanllama@gmail.com, @TartanLlama)
//
// Documentation available at https://tl.tartanllama.xyz/
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
///

#ifndef TL_OPTIONAL_HPP
#define TL_OPTIONAL_HPP

#define TL_OPTIONAL_VERSION_MAJOR 1
#define TL_OPTIONAL_VERSION_MINOR 0
#define TL_OPTIONAL_VERSION_PATCH 0

#include <exception>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

#if (defined(_MSC_VER) && _MSC_VER == 1900)
#define TL_OPTIONAL_MSVC2015
#endif

#if (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 9 &&              \
     !defined(__clang__))
#define TL_OPTIONAL_GCC49
#endif

#if (defined(__GNUC__) && __GNUC__ == 5 && __GNUC_MINOR__ <= 4 &&              \
     !defined(__clang__))
#define TL_OPTIONAL_GCC54
#endif

#if (defined(__GNUC__) && __GNUC__ == 5 && __GNUC_MINOR__ <= 5 &&              \
     !defined(__clang__))
#define TL_OPTIONAL_GCC55
#endif

#if (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 9 &&              \
     !defined(__clang__))
// GCC < 5 doesn't support overloading on const&& for member functions
#define TL_OPTIONAL_NO_CONSTRR

// GCC < 5 doesn't support some standard C++11 type traits
#define TL_OPTIONAL_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T)                                     \
  std::has_trivial_copy_constructor<T>::value
#define TL_OPTIONAL_IS_TRIVIALLY_COPY_ASSIGNABLE(T) std::has_trivial_copy_assign<T>::value

// This one will be different for GCC 5.7 if it's ever supported
#define TL_OPTIONAL_IS_TRIVIALLY_DESTRUCTIBLE(T) std::is_trivially_destructible<T>::value

// GCC 5 < v < 8 has a bug in is_trivially_copy_constructible which breaks std::vector
// for non-copyable types
#elif (defined(__GNUC__) && __GNUC__ < 8 &&                                                \
     !defined(__clang__))
#ifndef TL_GCC_LESS_8_TRIVIALLY_COPY_CONSTRUCTIBLE_MUTEX
#define TL_GCC_LESS_8_TRIVIALLY_COPY_CONSTRUCTIBLE_MUTEX
namespace tl {
  namespace detail {
      template<class T>
      struct is_trivially_copy_constructible : std::is_trivially_copy_constructible<T>{};
#ifdef _GLIBCXX_VECTOR
      template<class T, class A>
      struct is_trivially_copy_constructible<std::vector<T,A>>
          : std::is_trivially_copy_constructible<T>{};
#endif
  }
}
#endif

#define TL_OPTIONAL_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T)                                     \
    tl::detail::is_trivially_copy_constructible<T>::value
#define TL_OPTIONAL_IS_TRIVIALLY_COPY_ASSIGNABLE(T)                                        \
  std::is_trivially_copy_assignable<T>::value
#define TL_OPTIONAL_IS_TRIVIALLY_DESTRUCTIBLE(T) std::is_trivially_destructible<T>::value
#else
#define TL_OPTIONAL_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T)                                     \
  std::is_trivially_copy_constructible<T>::value
#define TL_OPTIONAL_IS_TRIVIALLY_COPY_ASSIGNABLE(T)                                        \
  std::is_trivially_copy_assignable<T>::value
#define TL_OPTIONAL_IS_TRIVIALLY_DESTRUCTIBLE(T) std::is_trivially_destructible<T>::value
#endif

#if __cplusplus > 201103L
#define TL_OPTIONAL_CXX14
#endif

// constexpr implies const in C++11, not C++14
#if (__cplusplus == 201103L || defined(TL_OPTIONAL_MSVC2015) ||                \
     defined(TL_OPTIONAL_GCC49))
#define TL_OPTIONAL_11_CONSTEXPR
#else
#define TL_OPTIONAL_11_CONSTEXPR constexpr
#endif

namespace tl {
#ifndef TL_MONOSTATE_INPLACE_MUTEX
#define TL_MONOSTATE_INPLACE_MUTEX
/// Used to represent an optional with no data; essentially a bool
class monostate {};

///  A tag type to tell optional to construct its value in-place
struct in_place_t {
  explicit in_place_t() = default;
};
/// A tag to tell optional to construct its value in-place
static constexpr in_place_t in_place{};
#endif

template <class T> class optional;

namespace detail {
#ifndef TL_TRAITS_MUTEX
#define TL_TRAITS_MUTEX
// C++14-style aliases for brevity
template <class T> using remove_const_t = typename std::remove_const<T>::type;
template <class T>
using remove_reference_t = typename std::remove_reference<T>::type;
template <class T> using decay_t = typename std::decay<T>::type;
template <bool E, class T = void>
using enable_if_t = typename std::enable_if<E, T>::type;
template <bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;

// std::conjunction from C++17
template <class...> struct conjunction : std::true_type {};
template <class B> struct conjunction<B> : B {};
template <class B, class... Bs>
struct conjunction<B, Bs...>
    : std::conditional<bool(B::value), conjunction<Bs...>, B>::type {};

#if defined(_LIBCPP_VERSION) && __cplusplus == 201103L
#define TL_TRAITS_LIBCXX_MEM_FN_WORKAROUND
#endif

// In C++11 mode, there's an issue in libc++'s std::mem_fn
// which results in a hard-error when using it in a noexcept expression
// in some cases. This is a check to workaround the common failing case.
#ifdef TL_TRAITS_LIBCXX_MEM_FN_WORKAROUND
template <class T> struct is_pointer_to_non_const_member_func : std::false_type{};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*) (Args...)> : std::true_type{};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*) (Args...)&> : std::true_type{};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*) (Args...)&&> : std::true_type{};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*) (Args...) volatile> : std::true_type{};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*) (Args...) volatile&> : std::true_type{};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*) (Args...) volatile&&> : std::true_type{};

template <class T> struct is_const_or_const_ref : std::false_type{};
template <class T> struct is_const_or_const_ref<T const&> : std::true_type{};
template <class T> struct is_const_or_const_ref<T const> : std::true_type{};
#endif

// std::invoke from C++17
// https://stackoverflow.com/questions/38288042/c11-14-invoke-workaround
template <typename Fn, typename... Args,
#ifdef TL_TRAITS_LIBCXX_MEM_FN_WORKAROUND
          typename = enable_if_t<!(is_pointer_to_non_const_member_func<Fn>::value
                                 && is_const_or_const_ref<Args...>::value)>,
#endif
          typename = enable_if_t<std::is_member_pointer<decay_t<Fn>>::value>,
          int = 0>
constexpr auto invoke(Fn &&f, Args &&... args) noexcept(
    noexcept(std::mem_fn(f)(std::forward<Args>(args)...)))
    -> decltype(std::mem_fn(f)(std::forward<Args>(args)...)) {
  return std::mem_fn(f)(std::forward<Args>(args)...);
}

template <typename Fn, typename... Args,
          typename = enable_if_t<!std::is_member_pointer<decay_t<Fn>>::value>>
constexpr auto invoke(Fn &&f, Args &&... args) noexcept(
    noexcept(std::forward<Fn>(f)(std::forward<Args>(args)...)))
    -> decltype(std::forward<Fn>(f)(std::forward<Args>(args)...)) {
  return std::forward<Fn>(f)(std::forward<Args>(args)...);
}

// std::invoke_result from C++17
template <class F, class, class... Us> struct invoke_result_impl;

template <class F, class... Us>
struct invoke_result_impl<
    F, decltype(detail::invoke(std::declval<F>(), std::declval<Us>()...), void()),
    Us...> {
  using type = decltype(detail::invoke(std::declval<F>(), std::declval<Us>()...));
};

template <class F, class... Us>
using invoke_result = invoke_result_impl<F, void, Us...>;

template <class F, class... Us>
using invoke_result_t = typename invoke_result<F, Us...>::type;

#if defined(_MSC_VER) && _MSC_VER <= 1900
// TODO make a version which works with MSVC 2015
template <class T, class U = T> struct is_swappable : std::true_type {};

template <class T, class U = T> struct is_nothrow_swappable : std::true_type {};
#else
// https://stackoverflow.com/questions/26744589/what-is-a-proper-way-to-implement-is-swappable-to-test-for-the-swappable-concept
namespace swap_adl_tests {
// if swap ADL finds this then it would call std::swap otherwise (same
// signature)
struct tag {};

template <class T> tag swap(T &, T &);
template <class T, std::size_t N> tag swap(T (&a)[N], T (&b)[N]);

// helper functions to test if an unqualified swap is possible, and if it
// becomes std::swap
template <class, class> std::false_type can_swap(...) noexcept(false);
template <class T, class U,
          class = decltype(swap(std::declval<T &>(), std::declval<U &>()))>
std::true_type can_swap(int) noexcept(noexcept(swap(std::declval<T &>(),
                                                    std::declval<U &>())));

template <class, class> std::false_type uses_std(...);
template <class T, class U>
std::is_same<decltype(swap(std::declval<T &>(), std::declval<U &>())), tag>
uses_std(int);

template <class T>
struct is_std_swap_noexcept
    : std::integral_constant<bool,
                             std::is_nothrow_move_constructible<T>::value &&
                                 std::is_nothrow_move_assignable<T>::value> {};

template <class T, std::size_t N>
struct is_std_swap_noexcept<T[N]> : is_std_swap_noexcept<T> {};

template <class T, class U>
struct is_adl_swap_noexcept
    : std::integral_constant<bool, noexcept(can_swap<T, U>(0))> {};
} // namespace swap_adl_tests

template <class T, class U = T>
struct is_swappable
    : std::integral_constant<
          bool,
          decltype(detail::swap_adl_tests::can_swap<T, U>(0))::value &&
              (!decltype(detail::swap_adl_tests::uses_std<T, U>(0))::value ||
               (std::is_move_assignable<T>::value &&
                std::is_move_constructible<T>::value))> {};

template <class T, std::size_t N>
struct is_swappable<T[N], T[N]>
    : std::integral_constant<
          bool,
          decltype(detail::swap_adl_tests::can_swap<T[N], T[N]>(0))::value &&
              (!decltype(
                   detail::swap_adl_tests::uses_std<T[N], T[N]>(0))::value ||
               is_swappable<T, T>::value)> {};

template <class T, class U = T>
struct is_nothrow_swappable
    : std::integral_constant<
          bool,
          is_swappable<T, U>::value &&
              ((decltype(detail::swap_adl_tests::uses_std<T, U>(0))::value
                    &&detail::swap_adl_tests::is_std_swap_noexcept<T>::value) ||
               (!decltype(detail::swap_adl_tests::uses_std<T, U>(0))::value &&
                    detail::swap_adl_tests::is_adl_swap_noexcept<T,
                                                                 U>::value))> {
};
#endif
#endif

// std::void_t from C++17
template <class...> struct voider { using type = void; };
template <class... Ts> using void_t = typename voider<Ts...>::type;

// Trait for checking if a type is a tl::optional
template <class T> struct is_optional_impl : std::false_type {};
template <class T> struct is_optional_impl<optional<T>> : std::true_type {};
template <class T> using is_optional = is_optional_impl<decay_t<T>>;

// Change void to tl::monostate
template <class U>
using fixup_void = conditional_t<std::is_void<U>::value, monostate, U>;

template <class F, class U, class = invoke_result_t<F, U>>
using get_map_return = optional<fixup_void<invoke_result_t<F, U>>>;

// Check if invoking F for some Us returns void
template <class F, class = void, class... U> struct returns_void_impl;
template <class F, class... U>
struct returns_void_impl<F, void_t<invoke_result_t<F, U...>>, U...>
    : std::is_void<invoke_result_t<F, U...>> {};
template <class F, class... U>
using returns_void = returns_void_impl<F, void, U...>;

template <class T, class... U>
using enable_if_ret_void = enable_if_t<returns_void<T &&, U...>::value>;

template <class T, class... U>
using disable_if_ret_void = enable_if_t<!returns_void<T &&, U...>::value>;

template <class T, class U>
using enable_forward_value =
    detail::enable_if_t<std::is_constructible<T, U &&>::value &&
                        !std::is_same<detail::decay_t<U>, in_place_t>::value &&
                        !std::is_same<optional<T>, detail::decay_t<U>>::value>;

template <class T, class U, class Other>
using enable_from_other = detail::enable_if_t<
    std::is_constructible<T, Other>::value &&
    !std::is_constructible<T, optional<U> &>::value &&
    !std::is_constructible<T, optional<U> &&>::value &&
    !std::is_constructible<T, const optional<U> &>::value &&
    !std::is_constructible<T, const optional<U> &&>::value &&
    !std::is_convertible<optional<U> &, T>::value &&
    !std::is_convertible<optional<U> &&, T>::value &&
    !std::is_convertible<const optional<U> &, T>::value &&
    !std::is_convertible<const optional<U> &&, T>::value>;

template <class T, class U>
using enable_assign_forward = detail::enable_if_t<
    !std::is_same<optional<T>, detail::decay_t<U>>::value &&
    !detail::conjunction<std::is_scalar<T>,
                         std::is_same<T, detail::decay_t<U>>>::value &&
    std::is_constructible<T, U>::value && std::is_assignable<T &, U>::value>;

template <class T, class U, class Other>
using enable_assign_from_other = detail::enable_if_t<
    std::is_constructible<T, Other>::value &&
    std::is_assignable<T &, Other>::value &&
    !std::is_constructible<T, optional<U> &>::value &&
    !std::is_constructible<T, optional<U> &&>::value &&
    !std::is_constructible<T, const optional<U> &>::value &&
    !std::is_constructible<T, const optional<U> &&>::value &&
    !std::is_convertible<optional<U> &, T>::value &&
    !std::is_convertible<optional<U> &&, T>::value &&
    !std::is_convertible<const optional<U> &, T>::value &&
    !std::is_convertible<const optional<U> &&, T>::value &&
    !std::is_assignable<T &, optional<U> &>::value &&
    !std::is_assignable<T &, optional<U> &&>::value &&
    !std::is_assignable<T &, const optional<U> &>::value &&
    !std::is_assignable<T &, const optional<U> &&>::value>;

// The storage base manages the actual storage, and correctly propagates
// trivial destruction from T. This case is for when T is not trivially
// destructible.
template <class T, bool = ::std::is_trivially_destructible<T>::value>
struct optional_storage_base {
  TL_OPTIONAL_11_CONSTEXPR optional_storage_base() noexcept
      : m_dummy(), m_has_value(false) {}

  template <class... U>
  TL_OPTIONAL_11_CONSTEXPR optional_storage_base(in_place_t, U &&... u)
      : m_value(std::forward<U>(u)...), m_has_value(true) {}

  ~optional_storage_base() {
    if (m_has_value) {
      m_value.~T();
      m_has_value = false;
    }
  }

  struct dummy {};
  union {
    dummy m_dummy;
    T m_value;
  };

  bool m_has_value;
};

// This case is for when T is trivially destructible.
template <class T> struct optional_storage_base<T, true> {
  TL_OPTIONAL_11_CONSTEXPR optional_storage_base() noexcept
      : m_dummy(), m_has_value(false) {}

  template <class... U>
  TL_OPTIONAL_11_CONSTEXPR optional_storage_base(in_place_t, U &&... u)
      : m_value(std::forward<U>(u)...), m_has_value(true) {}

  // No destructor, so this class is trivially destructible

  struct dummy {};
  union {
    dummy m_dummy;
    T m_value;
  };

  bool m_has_value = false;
};

// This base class provides some handy member functions which can be used in
// further derived classes
template <class T> struct optional_operations_base : optional_storage_base<T> {
  using optional_storage_base<T>::optional_storage_base;

  void hard_reset() noexcept {
    get().~T();
    this->m_has_value = false;
  }

  template <class... Args> void construct(Args &&... args) noexcept {
    new (std::addressof(this->m_value)) T(std::forward<Args>(args)...);
    this->m_has_value = true;
  }

  template <class Opt> void assign(Opt &&rhs) {
    if (this->has_value()) {
      if (rhs.has_value()) {
        this->m_value = std::forward<Opt>(rhs).get();
      } else {
        this->m_value.~T();
        this->m_has_value = false;
      }
    }

    else if (rhs.has_value()) {
      construct(std::forward<Opt>(rhs).get());
    }
  }

  bool has_value() const { return this->m_has_value; }

  TL_OPTIONAL_11_CONSTEXPR T &get() & { return this->m_value; }
  TL_OPTIONAL_11_CONSTEXPR const T &get() const & { return this->m_value; }
  TL_OPTIONAL_11_CONSTEXPR T &&get() && { return std::move(this->m_value); }
#ifndef TL_OPTIONAL_NO_CONSTRR
  constexpr const T &&get() const && { return std::move(this->m_value); }
#endif
};

// This class manages conditionally having a trivial copy constructor
// This specialization is for when T is trivially copy constructible
template <class T, bool = TL_OPTIONAL_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T)>
struct optional_copy_base : optional_operations_base<T> {
  using optional_operations_base<T>::optional_operations_base;
};

// This specialization is for when T is not trivially copy constructible
template <class T>
struct optional_copy_base<T, false> : optional_operations_base<T> {
  using optional_operations_base<T>::optional_operations_base;

  optional_copy_base() = default;
  optional_copy_base(const optional_copy_base &rhs)
  : optional_operations_base<T>() {
    if (rhs.has_value()) {
      this->construct(rhs.get());
    } else {
      this->m_has_value = false;
    }
  }

  optional_copy_base(optional_copy_base &&rhs) = default;
  optional_copy_base &operator=(const optional_copy_base &rhs) = default;
  optional_copy_base &operator=(optional_copy_base &&rhs) = default;
};

// This class manages conditionally having a trivial move constructor
// Unfortunately there's no way to achieve this in GCC < 5 AFAIK, since it
// doesn't implement an analogue to std::is_trivially_move_constructible. We
// have to make do with a non-trivial move constructor even if T is trivially
// move constructible
#ifndef TL_OPTIONAL_GCC49
template <class T, bool = std::is_trivially_move_constructible<T>::value>
struct optional_move_base : optional_copy_base<T> {
  using optional_copy_base<T>::optional_copy_base;
};
#else
template <class T, bool = false> struct optional_move_base;
#endif
template <class T> struct optional_move_base<T, false> : optional_copy_base<T> {
  using optional_copy_base<T>::optional_copy_base;

  optional_move_base() = default;
  optional_move_base(const optional_move_base &rhs) = default;

  optional_move_base(optional_move_base &&rhs) noexcept(
      std::is_nothrow_move_constructible<T>::value) {
    if (rhs.has_value()) {
      this->construct(std::move(rhs.get()));
    } else {
      this->m_has_value = false;
    }
  }
  optional_move_base &operator=(const optional_move_base &rhs) = default;
  optional_move_base &operator=(optional_move_base &&rhs) = default;
};

// This class manages conditionally having a trivial copy assignment operator
template <class T, bool = TL_OPTIONAL_IS_TRIVIALLY_COPY_ASSIGNABLE(T) &&
                          TL_OPTIONAL_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T) &&
                          TL_OPTIONAL_IS_TRIVIALLY_DESTRUCTIBLE(T)>
struct optional_copy_assign_base : optional_move_base<T> {
  using optional_move_base<T>::optional_move_base;
};

template <class T>
struct optional_copy_assign_base<T, false> : optional_move_base<T> {
  using optional_move_base<T>::optional_move_base;

  optional_copy_assign_base() = default;
  optional_copy_assign_base(const optional_copy_assign_base &rhs) = default;

  optional_copy_assign_base(optional_copy_assign_base &&rhs) = default;
  optional_copy_assign_base &operator=(const optional_copy_assign_base &rhs) {
    this->assign(rhs);
    return *this;
  }
  optional_copy_assign_base &
  operator=(optional_copy_assign_base &&rhs) = default;
};

// This class manages conditionally having a trivial move assignment operator
// Unfortunately there's no way to achieve this in GCC < 5 AFAIK, since it
// doesn't implement an analogue to std::is_trivially_move_assignable. We have
// to make do with a non-trivial move assignment operator even if T is trivially
// move assignable
#ifndef TL_OPTIONAL_GCC49
template <class T, bool = std::is_trivially_destructible<T>::value
                       &&std::is_trivially_move_constructible<T>::value
                           &&std::is_trivially_move_assignable<T>::value>
struct optional_move_assign_base : optional_copy_assign_base<T> {
  using optional_copy_assign_base<T>::optional_copy_assign_base;
};
#else
template <class T, bool = false> struct optional_move_assign_base;
#endif

template <class T>
struct optional_move_assign_base<T, false> : optional_copy_assign_base<T> {
  using optional_copy_assign_base<T>::optional_copy_assign_base;

  optional_move_assign_base() = default;
  optional_move_assign_base(const optional_move_assign_base &rhs) = default;

  optional_move_assign_base(optional_move_assign_base &&rhs) = default;

  optional_move_assign_base &
  operator=(const optional_move_assign_base &rhs) = default;

  optional_move_assign_base &
  operator=(optional_move_assign_base &&rhs) noexcept(
      std::is_nothrow_move_constructible<T>::value
          &&std::is_nothrow_move_assignable<T>::value) {
    this->assign(std::move(rhs));
    return *this;
  }
};

// optional_delete_ctor_base will conditionally delete copy and move
// constructors depending on whether T is copy/move constructible
template <class T, bool EnableCopy = std::is_copy_constructible<T>::value,
          bool EnableMove = std::is_move_constructible<T>::value>
struct optional_delete_ctor_base {
  optional_delete_ctor_base() = default;
  optional_delete_ctor_base(const optional_delete_ctor_base &) = default;
  optional_delete_ctor_base(optional_delete_ctor_base &&) noexcept = default;
  optional_delete_ctor_base &
  operator=(const optional_delete_ctor_base &) = default;
  optional_delete_ctor_base &
  operator=(optional_delete_ctor_base &&) noexcept = default;
};

template <class T> struct optional_delete_ctor_base<T, true, false> {
  optional_delete_ctor_base() = default;
  optional_delete_ctor_base(const optional_delete_ctor_base &) = default;
  optional_delete_ctor_base(optional_delete_ctor_base &&) noexcept = delete;
  optional_delete_ctor_base &
  operator=(const optional_delete_ctor_base &) = default;
  optional_delete_ctor_base &
  operator=(optional_delete_ctor_base &&) noexcept = default;
};

template <class T> struct optional_delete_ctor_base<T, false, true> {
  optional_delete_ctor_base() = default;
  optional_delete_ctor_base(const optional_delete_ctor_base &) = delete;
  optional_delete_ctor_base(optional_delete_ctor_base &&) noexcept = default;
  optional_delete_ctor_base &
  operator=(const optional_delete_ctor_base &) = default;
  optional_delete_ctor_base &
  operator=(optional_delete_ctor_base &&) noexcept = default;
};

template <class T> struct optional_delete_ctor_base<T, false, false> {
  optional_delete_ctor_base() = default;
  optional_delete_ctor_base(const optional_delete_ctor_base &) = delete;
  optional_delete_ctor_base(optional_delete_ctor_base &&) noexcept = delete;
  optional_delete_ctor_base &
  operator=(const optional_delete_ctor_base &) = default;
  optional_delete_ctor_base &
  operator=(optional_delete_ctor_base &&) noexcept = default;
};

// optional_delete_assign_base will conditionally delete copy and move
// constructors depending on whether T is copy/move constructible + assignable
template <class T,
          bool EnableCopy = (std::is_copy_constructible<T>::value &&
                             std::is_copy_assignable<T>::value),
          bool EnableMove = (std::is_move_constructible<T>::value &&
                             std::is_move_assignable<T>::value)>
struct optional_delete_assign_base {
  optional_delete_assign_base() = default;
  optional_delete_assign_base(const optional_delete_assign_base &) = default;
  optional_delete_assign_base(optional_delete_assign_base &&) noexcept =
      default;
  optional_delete_assign_base &
  operator=(const optional_delete_assign_base &) = default;
  optional_delete_assign_base &
  operator=(optional_delete_assign_base &&) noexcept = default;
};

template <class T> struct optional_delete_assign_base<T, true, false> {
  optional_delete_assign_base() = default;
  optional_delete_assign_base(const optional_delete_assign_base &) = default;
  optional_delete_assign_base(optional_delete_assign_base &&) noexcept =
      default;
  optional_delete_assign_base &
  operator=(const optional_delete_assign_base &) = default;
  optional_delete_assign_base &
  operator=(optional_delete_assign_base &&) noexcept = delete;
};

template <class T> struct optional_delete_assign_base<T, false, true> {
  optional_delete_assign_base() = default;
  optional_delete_assign_base(const optional_delete_assign_base &) = default;
  optional_delete_assign_base(optional_delete_assign_base &&) noexcept =
      default;
  optional_delete_assign_base &
  operator=(const optional_delete_assign_base &) = delete;
  optional_delete_assign_base &
  operator=(optional_delete_assign_base &&) noexcept = default;
};

template <class T> struct optional_delete_assign_base<T, false, false> {
  optional_delete_assign_base() = default;
  optional_delete_assign_base(const optional_delete_assign_base &) = default;
  optional_delete_assign_base(optional_delete_assign_base &&) noexcept =
      default;
  optional_delete_assign_base &
  operator=(const optional_delete_assign_base &) = delete;
  optional_delete_assign_base &
  operator=(optional_delete_assign_base &&) noexcept = delete;
};

} // namespace detail

/// A tag type to represent an empty optional
struct nullopt_t {
  struct do_not_use {};
  constexpr explicit nullopt_t(do_not_use, do_not_use) noexcept {}
};
/// Represents an empty optional
static constexpr nullopt_t nullopt{nullopt_t::do_not_use{},
                                   nullopt_t::do_not_use{}};

class bad_optional_access : public std::exception {
public:
  bad_optional_access() = default;
  const char *what() const noexcept { return "Optional has no value"; }
};

/// An optional object is an object that contains the storage for another
/// object and manages the lifetime of this contained object, if any. The
/// contained object may be initialized after the optional object has been
/// initialized, and may be destroyed before the optional object has been
/// destroyed. The initialization state of the contained object is tracked by
/// the optional object.
template <class T>
class optional : private detail::optional_move_assign_base<T>,
                 private detail::optional_delete_ctor_base<T>,
                 private detail::optional_delete_assign_base<T> {
  using base = detail::optional_move_assign_base<T>;

  static_assert(!std::is_same<T, in_place_t>::value,
                "instantiation of optional with in_place_t is ill-formed");
  static_assert(!std::is_same<detail::decay_t<T>, nullopt_t>::value,
                "instantiation of optional with nullopt_t is ill-formed");

public:
// The different versions for C++14 and 11 are needed because deduced return
// types are not SFINAE-safe. This provides better support for things like
// generic lambdas. C.f.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0826r0.html
#if defined(TL_OPTIONAL_CXX14) && !defined(TL_OPTIONAL_GCC49) &&               \
    !defined(TL_OPTIONAL_GCC54) && !defined(TL_OPTIONAL_GCC55)
  /// Carries out some operation which returns an optional on the stored
  /// object if there is one.
  template <class F> TL_OPTIONAL_11_CONSTEXPR auto and_then(F &&f) & {
    using result = detail::invoke_result_t<F, T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

  template <class F> TL_OPTIONAL_11_CONSTEXPR auto and_then(F &&f) && {
    using result = detail::invoke_result_t<F, T &&>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : result(nullopt);
  }

  template <class F> constexpr auto and_then(F &&f) const & {
    using result = detail::invoke_result_t<F, const T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F> constexpr auto and_then(F &&f) const && {
    using result = detail::invoke_result_t<F, const T &&>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : result(nullopt);
  }
#endif
#else
  /// Carries out some operation which returns an optional on the stored
  /// object if there is one.
  template <class F>
  TL_OPTIONAL_11_CONSTEXPR detail::invoke_result_t<F, T &> and_then(F &&f) & {
    using result = detail::invoke_result_t<F, T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

  template <class F>
  TL_OPTIONAL_11_CONSTEXPR detail::invoke_result_t<F, T &&> and_then(F &&f) && {
    using result = detail::invoke_result_t<F, T &&>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : result(nullopt);
  }

  template <class F>
  constexpr detail::invoke_result_t<F, const T &> and_then(F &&f) const & {
    using result = detail::invoke_result_t<F, const T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F>
  constexpr detail::invoke_result_t<F, const T &&> and_then(F &&f) const && {
    using result = detail::invoke_result_t<F, const T &&>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : result(nullopt);
  }
#endif
#endif

#if defined(TL_OPTIONAL_CXX14) && !defined(TL_OPTIONAL_GCC49) &&               \
    !defined(TL_OPTIONAL_GCC54) && !defined(TL_OPTIONAL_GCC55)
  /// Carries out some operation on the stored object if there is one.
  template <class F> TL_OPTIONAL_11_CONSTEXPR auto map(F &&f) & {
    return optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F> TL_OPTIONAL_11_CONSTEXPR auto map(F &&f) && {
    return optional_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F> constexpr auto map(F &&f) const & {
    return optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F> constexpr auto map(F &&f) const && {
    return optional_map_impl(std::move(*this), std::forward<F>(f));
  }
#else
  /// Carries out some operation on the stored object if there is one.
  template <class F>
  TL_OPTIONAL_11_CONSTEXPR decltype(optional_map_impl(std::declval<optional &>(),
                                             std::declval<F &&>()))
  map(F &&f) & {
    return optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F>
  TL_OPTIONAL_11_CONSTEXPR decltype(optional_map_impl(std::declval<optional &&>(),
                                             std::declval<F &&>()))
  map(F &&f) && {
    return optional_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F>
  constexpr decltype(optional_map_impl(std::declval<const optional &>(),
                              std::declval<F &&>()))
  map(F &&f) const & {
    return optional_map_impl(*this, std::forward<F>(f));
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F>
  constexpr decltype(optional_map_impl(std::declval<const optional &&>(),
                              std::declval<F &&>()))
  map(F &&f) const && {
    return optional_map_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif

#if defined(TL_OPTIONAL_CXX14) && !defined(TL_OPTIONAL_GCC49) &&               \
    !defined(TL_OPTIONAL_GCC54) && !defined(TL_OPTIONAL_GCC55)
  /// Carries out some operation on the stored object if there is one.
  template <class F> TL_OPTIONAL_11_CONSTEXPR auto transform(F&& f) & {
    return optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F> TL_OPTIONAL_11_CONSTEXPR auto transform(F&& f) && {
    return optional_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F> constexpr auto transform(F&& f) const & {
    return optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F> constexpr auto transform(F&& f) const && {
    return optional_map_impl(std::move(*this), std::forward<F>(f));
  }
#else
  /// Carries out some operation on the stored object if there is one.
  template <class F>
  TL_OPTIONAL_11_CONSTEXPR decltype(optional_map_impl(std::declval<optional&>(),
    std::declval<F&&>()))
    transform(F&& f) & {
    return optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F>
  TL_OPTIONAL_11_CONSTEXPR decltype(optional_map_impl(std::declval<optional&&>(),
    std::declval<F&&>()))
    transform(F&& f) && {
    return optional_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F>
  constexpr decltype(optional_map_impl(std::declval<const optional&>(),
    std::declval<F&&>()))
    transform(F&& f) const & {
    return optional_map_impl(*this, std::forward<F>(f));
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F>
  constexpr decltype(optional_map_impl(std::declval<const optional&&>(),
    std::declval<F&&>()))
    transform(F&& f) const && {
    return optional_map_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif

  /// Calls `f` if the optional is empty
  template <class F, detail::enable_if_ret_void<F> * = nullptr>
  optional<T> TL_OPTIONAL_11_CONSTEXPR or_else(F &&f) & {
    if (has_value())
      return *this;

    std::forward<F>(f)();
    return nullopt;
  }

  template <class F, detail::disable_if_ret_void<F> * = nullptr>
  optional<T> TL_OPTIONAL_11_CONSTEXPR or_else(F &&f) & {
    return has_value() ? *this : std::forward<F>(f)();
  }

  template <class F, detail::enable_if_ret_void<F> * = nullptr>
  optional<T> or_else(F &&f) && {
    if (has_value())
      return std::move(*this);

    std::forward<F>(f)();
    return nullopt;
  }

  template <class F, detail::disable_if_ret_void<F> * = nullptr>
  optional<T> TL_OPTIONAL_11_CONSTEXPR or_else(F &&f) && {
    return has_value() ? std::move(*this) : std::forward<F>(f)();
  }

  template <class F, detail::enable_if_ret_void<F> * = nullptr>
  optional<T> or_else(F &&f) const & {
    if (has_value())
      return *this;

    std::forward<F>(f)();
    return nullopt;
  }

  template <class F, detail::disable_if_ret_void<F> * = nullptr>
  optional<T> TL_OPTIONAL_11_CONSTEXPR or_else(F &&f) const & {
    return has_value() ? *this : std::forward<F>(f)();
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F, detail::enable_if_ret_void<F> * = nullptr>
  optional<T> or_else(F &&f) const && {
    if (has_value())
      return std::move(*this);

    std::forward<F>(f)();
    return nullopt;
  }

  template <class F, detail::disable_if_ret_void<F> * = nullptr>
  optional<T> or_else(F &&f) const && {
    return has_value() ? std::move(*this) : std::forward<F>(f)();
  }
#endif

  /// Maps the stored value with `f` if there is one, otherwise returns `u`.
  template <class F, class U> U map_or(F &&f, U &&u) & {
    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : std::forward<U>(u);
  }

  template <class F, class U> U map_or(F &&f, U &&u) && {
    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : std::forward<U>(u);
  }

  template <class F, class U> U map_or(F &&f, U &&u) const & {
    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : std::forward<U>(u);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F, class U> U map_or(F &&f, U &&u) const && {
    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : std::forward<U>(u);
  }
#endif

  /// Maps the stored value with `f` if there is one, otherwise calls
  /// `u` and returns the result.
  template <class F, class U>
  detail::invoke_result_t<U> map_or_else(F &&f, U &&u) & {
    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : std::forward<U>(u)();
  }

  template <class F, class U>
  detail::invoke_result_t<U> map_or_else(F &&f, U &&u) && {
    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : std::forward<U>(u)();
  }

  template <class F, class U>
  detail::invoke_result_t<U> map_or_else(F &&f, U &&u) const & {
    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : std::forward<U>(u)();
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F, class U>
  detail::invoke_result_t<U> map_or_else(F &&f, U &&u) const && {
    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : std::forward<U>(u)();
  }
#endif

  /// Returns `u` if `*this` has a value, otherwise an empty optional.
  template <class U>
  constexpr optional<typename std::decay<U>::type> conjunction(U &&u) const {
    using result = optional<detail::decay_t<U>>;
    return has_value() ? result{u} : result{nullopt};
  }

  /// Returns `rhs` if `*this` is empty, otherwise the current value.
  TL_OPTIONAL_11_CONSTEXPR optional disjunction(const optional &rhs) & {
    return has_value() ? *this : rhs;
  }

  constexpr optional disjunction(const optional &rhs) const & {
    return has_value() ? *this : rhs;
  }

  TL_OPTIONAL_11_CONSTEXPR optional disjunction(const optional &rhs) && {
    return has_value() ? std::move(*this) : rhs;
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  constexpr optional disjunction(const optional &rhs) const && {
    return has_value() ? std::move(*this) : rhs;
  }
#endif

  TL_OPTIONAL_11_CONSTEXPR optional disjunction(optional &&rhs) & {
    return has_value() ? *this : std::move(rhs);
  }

  constexpr optional disjunction(optional &&rhs) const & {
    return has_value() ? *this : std::move(rhs);
  }

  TL_OPTIONAL_11_CONSTEXPR optional disjunction(optional &&rhs) && {
    return has_value() ? std::move(*this) : std::move(rhs);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  constexpr optional disjunction(optional &&rhs) const && {
    return has_value() ? std::move(*this) : std::move(rhs);
  }
#endif

  /// Takes the value out of the optional, leaving it empty
  optional take() {
    optional ret = std::move(*this);
    reset();
    return ret;
  }

  using value_type = T;

  /// Constructs an optional that does not contain a value.
  constexpr optional() noexcept = default;

  constexpr optional(nullopt_t) noexcept {}

  /// Copy constructor
  ///
  /// If `rhs` contains a value, the stored value is direct-initialized with
  /// it. Otherwise, the constructed optional is empty.
  TL_OPTIONAL_11_CONSTEXPR optional(const optional &rhs) = default;

  /// Move constructor
  ///
  /// If `rhs` contains a value, the stored value is direct-initialized with
  /// it. Otherwise, the constructed optional is empty.
  TL_OPTIONAL_11_CONSTEXPR optional(optional &&rhs) = default;

  /// Constructs the stored value in-place using the given arguments.
 template <class... Args>
  constexpr explicit optional(
      detail::enable_if_t<std::is_constructible<T, Args...>::value, in_place_t>,
      Args &&... args)
      : base(in_place, std::forward<Args>(args)...) {}

  template <class U, class... Args>
  TL_OPTIONAL_11_CONSTEXPR explicit optional(
      detail::enable_if_t<std::is_constructible<T, std::initializer_list<U> &,
                                                Args &&...>::value,
                          in_place_t>,
      std::initializer_list<U> il, Args &&... args) {
    this->construct(il, std::forward<Args>(args)...);
  }

  /// Constructs the stored value with `u`.
  template <
      class U = T,
      detail::enable_if_t<std::is_convertible<U &&, T>::value> * = nullptr,
      detail::enable_forward_value<T, U> * = nullptr>
  constexpr optional(U &&u) : base(in_place, std::forward<U>(u)) {}

  template <
      class U = T,
      detail::enable_if_t<!std::is_convertible<U &&, T>::value> * = nullptr,
      detail::enable_forward_value<T, U> * = nullptr>
  constexpr explicit optional(U &&u) : base(in_place, std::forward<U>(u)) {}

  /// Converting copy constructor.
  template <
      class U, detail::enable_from_other<T, U, const U &> * = nullptr,
      detail::enable_if_t<std::is_convertible<const U &, T>::value> * = nullptr>
  optional(const optional<U> &rhs) {
    if (rhs.has_value()) {
      this->construct(*rhs);
    }
  }

  template <class U, detail::enable_from_other<T, U, const U &> * = nullptr,
            detail::enable_if_t<!std::is_convertible<const U &, T>::value> * =
                nullptr>
  explicit optional(const optional<U> &rhs) {
    if (rhs.has_value()) {
      this->construct(*rhs);
    }
  }

  /// Converting move constructor.
  template <
      class U, detail::enable_from_other<T, U, U &&> * = nullptr,
      detail::enable_if_t<std::is_convertible<U &&, T>::value> * = nullptr>
  optional(optional<U> &&rhs) {
    if (rhs.has_value()) {
      this->construct(std::move(*rhs));
    }
  }

  template <
      class U, detail::enable_from_other<T, U, U &&> * = nullptr,
      detail::enable_if_t<!std::is_convertible<U &&, T>::value> * = nullptr>
  explicit optional(optional<U> &&rhs) {
    if (rhs.has_value()) {
      this->construct(std::move(*rhs));
    }
  }

  /// Destroys the stored value if there is one.
  ~optional() = default;

  /// Assignment to empty.
  ///
  /// Destroys the current value if there is one.
  optional &operator=(nullopt_t) noexcept {
    if (has_value()) {
      this->m_value.~T();
      this->m_has_value = false;
    }

    return *this;
  }

  /// Copy assignment.
  ///
  /// Copies the value from `rhs` if there is one. Otherwise resets the stored
  /// value in `*this`.
  optional &operator=(const optional &rhs) = default;

  /// Move assignment.
  ///
  /// Moves the value from `rhs` if there is one. Otherwise resets the stored
  /// value in `*this`.
  optional &operator=(optional &&rhs) = default;

  /// Assigns the stored value from `u`, destroying the old value if there was
  /// one.
  template <class U = T, detail::enable_assign_forward<T, U> * = nullptr>
  optional &operator=(U &&u) {
    if (has_value()) {
      this->m_value = std::forward<U>(u);
    } else {
      this->construct(std::forward<U>(u));
    }

    return *this;
  }

  /// Converting copy assignment operator.
  ///
  /// Copies the value from `rhs` if there is one. Otherwise resets the stored
  /// value in `*this`.
  template <class U,
            detail::enable_assign_from_other<T, U, const U &> * = nullptr>
  optional &operator=(const optional<U> &rhs) {
    if (has_value()) {
      if (rhs.has_value()) {
        this->m_value = *rhs;
      } else {
        this->hard_reset();
      }
    }

    if (rhs.has_value()) {
      this->construct(*rhs);
    }

    return *this;
  }

  // TODO check exception guarantee
  /// Converting move assignment operator.
  ///
  /// Moves the value from `rhs` if there is one. Otherwise resets the stored
  /// value in `*this`.
  template <class U, detail::enable_assign_from_other<T, U, U> * = nullptr>
  optional &operator=(optional<U> &&rhs) {
    if (has_value()) {
      if (rhs.has_value()) {
        this->m_value = std::move(*rhs);
      } else {
        this->hard_reset();
      }
    }

    if (rhs.has_value()) {
      this->construct(std::move(*rhs));
    }

    return *this;
  }

  /// Constructs the value in-place, destroying the current one if there is
  /// one.
  template <class... Args> T &emplace(Args &&... args) {
    static_assert(std::is_constructible<T, Args &&...>::value,
                  "T must be constructible with Args");

    *this = nullopt;
    this->construct(std::forward<Args>(args)...);
    return value();
  }

  template <class U, class... Args>
  detail::enable_if_t<
      std::is_constructible<T, std::initializer_list<U> &, Args &&...>::value,
      T &>
  emplace(std::initializer_list<U> il, Args &&... args) {
    *this = nullopt;
    this->construct(il, std::forward<Args>(args)...);
    return value();
  }

  /// Swaps this optional with the other.
  ///
  /// If neither optionals have a value, nothing happens.
  /// If both have a value, the values are swapped.
  /// If one has a value, it is moved to the other and the movee is left
  /// valueless.
  void
  swap(optional &rhs) noexcept(std::is_nothrow_move_constructible<T>::value
                                   &&detail::is_nothrow_swappable<T>::value) {
    using std::swap;
    if (has_value()) {
      if (rhs.has_value()) {
        swap(**this, *rhs);
      } else {
        new (std::addressof(rhs.m_value)) T(std::move(this->m_value));
        this->m_value.T::~T();
      }
    } else if (rhs.has_value()) {
      new (std::addressof(this->m_value)) T(std::move(rhs.m_value));
      rhs.m_value.T::~T();
    }
    swap(this->m_has_value, rhs.m_has_value);
  }

  /// Returns a pointer to the stored value
  constexpr const T *operator->() const {
    return std::addressof(this->m_value);
  }

  TL_OPTIONAL_11_CONSTEXPR T *operator->() {
    return std::addressof(this->m_value);
  }

  /// Returns the stored value
  TL_OPTIONAL_11_CONSTEXPR T &operator*() & { return this->m_value; }

  constexpr const T &operator*() const & { return this->m_value; }

  TL_OPTIONAL_11_CONSTEXPR T &&operator*() && {
    return std::move(this->m_value);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  constexpr const T &&operator*() const && { return std::move(this->m_value); }
#endif

  /// Returns whether or not the optional has a value
  constexpr bool has_value() const noexcept { return this->m_has_value; }

  constexpr explicit operator bool() const noexcept {
    return this->m_has_value;
  }

  /// Returns the contained value if there is one, otherwise throws bad_optional_access
  TL_OPTIONAL_11_CONSTEXPR T &value() & {
    if (has_value())
      return this->m_value;
    throw bad_optional_access();
  }
  TL_OPTIONAL_11_CONSTEXPR const T &value() const & {
    if (has_value())
      return this->m_value;
    throw bad_optional_access();
  }
  TL_OPTIONAL_11_CONSTEXPR T &&value() && {
    if (has_value())
      return std::move(this->m_value);
    throw bad_optional_access();
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  TL_OPTIONAL_11_CONSTEXPR const T &&value() const && {
    if (has_value())
      return std::move(this->m_value);
    throw bad_optional_access();
  }
#endif

  /// Returns the stored value if there is one, otherwise returns `u`
  template <class U> constexpr T value_or(U &&u) const & {
    static_assert(std::is_copy_constructible<T>::value &&
                      std::is_convertible<U &&, T>::value,
                  "T must be copy constructible and convertible from U");
    return has_value() ? **this : static_cast<T>(std::forward<U>(u));
  }

  template <class U> TL_OPTIONAL_11_CONSTEXPR T value_or(U &&u) && {
    static_assert(std::is_move_constructible<T>::value &&
                      std::is_convertible<U &&, T>::value,
                  "T must be move constructible and convertible from U");
    return has_value() ? **this : static_cast<T>(std::forward<U>(u));
  }

  /// Destroys the stored value if one exists, making the optional empty
  void reset() noexcept {
    if (has_value()) {
      this->m_value.~T();
      this->m_has_value = false;
    }
  }
}; // namespace tl

/// Compares two optional objects
template <class T, class U>
inline constexpr bool operator==(const optional<T> &lhs,
                                 const optional<U> &rhs) {
  return lhs.has_value() == rhs.has_value() &&
         (!lhs.has_value() || *lhs == *rhs);
}
template <class T, class U>
inline constexpr bool operator!=(const optional<T> &lhs,
                                 const optional<U> &rhs) {
  return lhs.has_value() != rhs.has_value() ||
         (lhs.has_value() && *lhs != *rhs);
}
template <class T, class U>
inline constexpr bool operator<(const optional<T> &lhs,
                                const optional<U> &rhs) {
  return rhs.has_value() && (!lhs.has_value() || *lhs < *rhs);
}
template <class T, class U>
inline constexpr bool operator>(const optional<T> &lhs,
                                const optional<U> &rhs) {
  return lhs.has_value() && (!rhs.has_value() || *lhs > *rhs);
}
template <class T, class U>
inline constexpr bool operator<=(const optional<T> &lhs,
                                 const optional<U> &rhs) {
  return !lhs.has_value() || (rhs.has_value() && *lhs <= *rhs);
}
template <class T, class U>
inline constexpr bool operator>=(const optional<T> &lhs,
                                 const optional<U> &rhs) {
  return !rhs.has_value() || (lhs.has_value() && *lhs >= *rhs);
}

/// Compares an optional to a `nullopt`
template <class T>
inline constexpr bool operator==(const optional<T> &lhs, nullopt_t) noexcept {
  return !lhs.has_value();
}
template <class T>
inline constexpr bool operator==(nullopt_t, const optional<T> &rhs) noexcept {
  return !rhs.has_value();
}
template <class T>
inline constexpr bool operator!=(const optional<T> &lhs, nullopt_t) noexcept {
  return lhs.has_value();
}
template <class T>
inline constexpr bool operator!=(nullopt_t, const optional<T> &rhs) noexcept {
  return rhs.has_value();
}
template <class T>
inline constexpr bool operator<(const optional<T> &, nullopt_t) noexcept {
  return false;
}
template <class T>
inline constexpr bool operator<(nullopt_t, const optional<T> &rhs) noexcept {
  return rhs.has_value();
}
template <class T>
inline constexpr bool operator<=(const optional<T> &lhs, nullopt_t) noexcept {
  return !lhs.has_value();
}
template <class T>
inline constexpr bool operator<=(nullopt_t, const optional<T> &) noexcept {
  return true;
}
template <class T>
inline constexpr bool operator>(const optional<T> &lhs, nullopt_t) noexcept {
  return lhs.has_value();
}
template <class T>
inline constexpr bool operator>(nullopt_t, const optional<T> &) noexcept {
  return false;
}
template <class T>
inline constexpr bool operator>=(const optional<T> &, nullopt_t) noexcept {
  return true;
}
template <class T>
inline constexpr bool operator>=(nullopt_t, const optional<T> &rhs) noexcept {
  return !rhs.has_value();
}

/// Compares the optional with a value.
template <class T, class U>
inline constexpr bool operator==(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs == rhs : false;
}
template <class T, class U>
inline constexpr bool operator==(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs == *rhs : false;
}
template <class T, class U>
inline constexpr bool operator!=(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs != rhs : true;
}
template <class T, class U>
inline constexpr bool operator!=(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs != *rhs : true;
}
template <class T, class U>
inline constexpr bool operator<(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs < rhs : true;
}
template <class T, class U>
inline constexpr bool operator<(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs < *rhs : false;
}
template <class T, class U>
inline constexpr bool operator<=(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs <= rhs : true;
}
template <class T, class U>
inline constexpr bool operator<=(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs <= *rhs : false;
}
template <class T, class U>
inline constexpr bool operator>(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs > rhs : false;
}
template <class T, class U>
inline constexpr bool operator>(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs > *rhs : true;
}
template <class T, class U>
inline constexpr bool operator>=(const optional<T> &lhs, const U &rhs) {
  return lhs.has_value() ? *lhs >= rhs : false;
}
template <class T, class U>
inline constexpr bool operator>=(const U &lhs, const optional<T> &rhs) {
  return rhs.has_value() ? lhs >= *rhs : true;
}

template <class T,
          detail::enable_if_t<std::is_move_constructible<T>::value> * = nullptr,
          detail::enable_if_t<detail::is_swappable<T>::value> * = nullptr>
void swap(optional<T> &lhs,
          optional<T> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  return lhs.swap(rhs);
}

namespace detail {
struct i_am_secret {};
} // namespace detail

template <class T = detail::i_am_secret, class U,
          class Ret =
              detail::conditional_t<std::is_same<T, detail::i_am_secret>::value,
                                    detail::decay_t<U>, T>>
inline constexpr optional<Ret> make_optional(U &&v) {
  return optional<Ret>(std::forward<U>(v));
}

template <class T, class... Args>
inline constexpr optional<T> make_optional(Args &&... args) {
  return optional<T>(in_place, std::forward<Args>(args)...);
}
template <class T, class U, class... Args>
inline constexpr optional<T> make_optional(std::initializer_list<U> il,
                                           Args &&... args) {
  return optional<T>(in_place, il, std::forward<Args>(args)...);
}

#if __cplusplus >= 201703L
template <class T> optional(T)->optional<T>;
#endif

/// \exclude
namespace detail {
#ifdef TL_OPTIONAL_CXX14
template <class Opt, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Opt>())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
constexpr auto optional_map_impl(Opt &&opt, F &&f) {
  return opt.has_value()
             ? detail::invoke(std::forward<F>(f), *std::forward<Opt>(opt))
             : optional<Ret>(nullopt);
}

template <class Opt, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Opt>())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
auto optional_map_impl(Opt &&opt, F &&f) {
  if (opt.has_value()) {
    detail::invoke(std::forward<F>(f), *std::forward<Opt>(opt));
    return make_optional(monostate{});
  }

  return optional<monostate>(nullopt);
}
#else
template <class Opt, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Opt>())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>

constexpr auto optional_map_impl(Opt &&opt, F &&f) -> optional<Ret> {
  return opt.has_value()
             ? detail::invoke(std::forward<F>(f), *std::forward<Opt>(opt))
             : optional<Ret>(nullopt);
}

template <class Opt, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Opt>())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>

auto optional_map_impl(Opt &&opt, F &&f) -> optional<monostate> {
  if (opt.has_value()) {
    detail::invoke(std::forward<F>(f), *std::forward<Opt>(opt));
    return monostate{};
  }

  return nullopt;
}
#endif
} // namespace detail

/// Specialization for when `T` is a reference. `optional<T&>` acts similarly
/// to a `T*`, but provides more operations and shows intent more clearly.
template <class T> class optional<T &> {
public:
// The different versions for C++14 and 11 are needed because deduced return
// types are not SFINAE-safe. This provides better support for things like
// generic lambdas. C.f.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0826r0.html
#if defined(TL_OPTIONAL_CXX14) && !defined(TL_OPTIONAL_GCC49) &&               \
    !defined(TL_OPTIONAL_GCC54) && !defined(TL_OPTIONAL_GCC55)

  /// Carries out some operation which returns an optional on the stored
  /// object if there is one.
  template <class F> TL_OPTIONAL_11_CONSTEXPR auto and_then(F &&f) & {
    using result = detail::invoke_result_t<F, T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

  template <class F> TL_OPTIONAL_11_CONSTEXPR auto and_then(F &&f) && {
    using result = detail::invoke_result_t<F, T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

  template <class F> constexpr auto and_then(F &&f) const & {
    using result = detail::invoke_result_t<F, const T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F> constexpr auto and_then(F &&f) const && {
    using result = detail::invoke_result_t<F, const T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }
#endif
#else
  /// Carries out some operation which returns an optional on the stored
  /// object if there is one.
  template <class F>
  TL_OPTIONAL_11_CONSTEXPR detail::invoke_result_t<F, T &> and_then(F &&f) & {
    using result = detail::invoke_result_t<F, T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

  template <class F>
  TL_OPTIONAL_11_CONSTEXPR detail::invoke_result_t<F, T &> and_then(F &&f) && {
    using result = detail::invoke_result_t<F, T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

  template <class F>
  constexpr detail::invoke_result_t<F, const T &> and_then(F &&f) const & {
    using result = detail::invoke_result_t<F, const T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F>
  constexpr detail::invoke_result_t<F, const T &> and_then(F &&f) const && {
    using result = detail::invoke_result_t<F, const T &>;
    static_assert(detail::is_optional<result>::value,
                  "F must return an optional");

    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : result(nullopt);
  }
#endif
#endif

#if defined(TL_OPTIONAL_CXX14) && !defined(TL_OPTIONAL_GCC49) &&               \
    !defined(TL_OPTIONAL_GCC54) && !defined(TL_OPTIONAL_GCC55)
  /// Carries out some operation on the stored object if there is one.
  template <class F> TL_OPTIONAL_11_CONSTEXPR auto map(F &&f) & {
    return detail::optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F> TL_OPTIONAL_11_CONSTEXPR auto map(F &&f) && {
    return detail::optional_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F> constexpr auto map(F &&f) const & {
    return detail::optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F> constexpr auto map(F &&f) const && {
    return detail::optional_map_impl(std::move(*this), std::forward<F>(f));
  }
#else
  /// Carries out some operation on the stored object if there is one.
  template <class F>
  TL_OPTIONAL_11_CONSTEXPR decltype(detail::optional_map_impl(std::declval<optional &>(),
                                                     std::declval<F &&>()))
  map(F &&f) & {
    return detail::optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F>
  TL_OPTIONAL_11_CONSTEXPR decltype(detail::optional_map_impl(std::declval<optional &&>(),
                                                     std::declval<F &&>()))
  map(F &&f) && {
    return detail::optional_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F>
  constexpr decltype(detail::optional_map_impl(std::declval<const optional &>(),
                                      std::declval<F &&>()))
  map(F &&f) const & {
    return detail::optional_map_impl(*this, std::forward<F>(f));
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F>
  constexpr decltype(detail::optional_map_impl(std::declval<const optional &&>(),
                                      std::declval<F &&>()))
  map(F &&f) const && {
    return detail::optional_map_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif

#if defined(TL_OPTIONAL_CXX14) && !defined(TL_OPTIONAL_GCC49) &&               \
    !defined(TL_OPTIONAL_GCC54) && !defined(TL_OPTIONAL_GCC55)
  /// Carries out some operation on the stored object if there is one.
  template <class F> TL_OPTIONAL_11_CONSTEXPR auto transform(F&& f) & {
    return detail::optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F> TL_OPTIONAL_11_CONSTEXPR auto transform(F&& f) && {
    return detail::optional_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F> constexpr auto transform(F&& f) const & {
    return detail::optional_map_impl(*this, std::forward<F>(f));
  }

  template <class F> constexpr auto transform(F&& f) const && {
    return detail::optional_map_impl(std::move(*this), std::forward<F>(f));
  }
#else
  /// Carries out some operation on the stored object if there is one.
  template <class F>
  TL_OPTIONAL_11_CONSTEXPR decltype(detail::optional_map_impl(std::declval<optional&>(),
    std::declval<F&&>()))
    transform(F&& f) & {
    return detail::optional_map_impl(*this, std::forward<F>(f));
  }

  /// \group map
  /// \synopsis template <class F> auto transform(F &&f) &&;
  template <class F>
  TL_OPTIONAL_11_CONSTEXPR decltype(detail::optional_map_impl(std::declval<optional&&>(),
    std::declval<F&&>()))
    transform(F&& f) && {
    return detail::optional_map_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F>
  constexpr decltype(detail::optional_map_impl(std::declval<const optional&>(),
    std::declval<F&&>()))
    transform(F&& f) const & {
    return detail::optional_map_impl(*this, std::forward<F>(f));
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F>
  constexpr decltype(detail::optional_map_impl(std::declval<const optional&&>(),
    std::declval<F&&>()))
    transform(F&& f) const && {
    return detail::optional_map_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif

  /// Calls `f` if the optional is empty
  template <class F, detail::enable_if_ret_void<F> * = nullptr>
  optional<T> TL_OPTIONAL_11_CONSTEXPR or_else(F &&f) & {
    if (has_value())
      return *this;

    std::forward<F>(f)();
    return nullopt;
  }

  template <class F, detail::disable_if_ret_void<F> * = nullptr>
  optional<T> TL_OPTIONAL_11_CONSTEXPR or_else(F &&f) & {
    return has_value() ? *this : std::forward<F>(f)();
  }

  template <class F, detail::enable_if_ret_void<F> * = nullptr>
  optional<T> or_else(F &&f) && {
    if (has_value())
      return std::move(*this);

    std::forward<F>(f)();
    return nullopt;
  }

  template <class F, detail::disable_if_ret_void<F> * = nullptr>
  optional<T> TL_OPTIONAL_11_CONSTEXPR or_else(F &&f) && {
    return has_value() ? std::move(*this) : std::forward<F>(f)();
  }

  template <class F, detail::enable_if_ret_void<F> * = nullptr>
  optional<T> or_else(F &&f) const & {
    if (has_value())
      return *this;

    std::forward<F>(f)();
    return nullopt;
  }

  template <class F, detail::disable_if_ret_void<F> * = nullptr>
  optional<T> TL_OPTIONAL_11_CONSTEXPR or_else(F &&f) const & {
    return has_value() ? *this : std::forward<F>(f)();
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F, detail::enable_if_ret_void<F> * = nullptr>
  optional<T> or_else(F &&f) const && {
    if (has_value())
      return std::move(*this);

    std::forward<F>(f)();
    return nullopt;
  }

  template <class F, detail::disable_if_ret_void<F> * = nullptr>
  optional<T> or_else(F &&f) const && {
    return has_value() ? std::move(*this) : std::forward<F>(f)();
  }
#endif

  /// Maps the stored value with `f` if there is one, otherwise returns `u`
  template <class F, class U> U map_or(F &&f, U &&u) & {
    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : std::forward<U>(u);
  }

  template <class F, class U> U map_or(F &&f, U &&u) && {
    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : std::forward<U>(u);
  }

  template <class F, class U> U map_or(F &&f, U &&u) const & {
    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : std::forward<U>(u);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F, class U> U map_or(F &&f, U &&u) const && {
    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : std::forward<U>(u);
  }
#endif

  /// Maps the stored value with `f` if there is one, otherwise calls
  /// `u` and returns the result.
  template <class F, class U>
  detail::invoke_result_t<U> map_or_else(F &&f, U &&u) & {
    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : std::forward<U>(u)();
  }

  template <class F, class U>
  detail::invoke_result_t<U> map_or_else(F &&f, U &&u) && {
    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : std::forward<U>(u)();
  }

  template <class F, class U>
  detail::invoke_result_t<U> map_or_else(F &&f, U &&u) const & {
    return has_value() ? detail::invoke(std::forward<F>(f), **this)
                       : std::forward<U>(u)();
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  template <class F, class U>
  detail::invoke_result_t<U> map_or_else(F &&f, U &&u) const && {
    return has_value() ? detail::invoke(std::forward<F>(f), std::move(**this))
                       : std::forward<U>(u)();
  }
#endif

  /// Returns `u` if `*this` has a value, otherwise an empty optional.
  template <class U>
  constexpr optional<typename std::decay<U>::type> conjunction(U &&u) const {
    using result = optional<detail::decay_t<U>>;
    return has_value() ? result{u} : result{nullopt};
  }

  /// Returns `rhs` if `*this` is empty, otherwise the current value.
  TL_OPTIONAL_11_CONSTEXPR optional disjunction(const optional &rhs) & {
    return has_value() ? *this : rhs;
  }

  constexpr optional disjunction(const optional &rhs) const & {
    return has_value() ? *this : rhs;
  }

  TL_OPTIONAL_11_CONSTEXPR optional disjunction(const optional &rhs) && {
    return has_value() ? std::move(*this) : rhs;
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  constexpr optional disjunction(const optional &rhs) const && {
    return has_value() ? std::move(*this) : rhs;
  }
#endif

  TL_OPTIONAL_11_CONSTEXPR optional disjunction(optional &&rhs) & {
    return has_value() ? *this : std::move(rhs);
  }

  constexpr optional disjunction(optional &&rhs) const & {
    return has_value() ? *this : std::move(rhs);
  }

  TL_OPTIONAL_11_CONSTEXPR optional disjunction(optional &&rhs) && {
    return has_value() ? std::move(*this) : std::move(rhs);
  }

#ifndef TL_OPTIONAL_NO_CONSTRR
  constexpr optional disjunction(optional &&rhs) const && {
    return has_value() ? std::move(*this) : std::move(rhs);
  }
#endif

  /// Takes the value out of the optional, leaving it empty
  optional take() {
    optional ret = std::move(*this);
    reset();
    return ret;
  }

  using value_type = T &;

  /// Constructs an optional that does not contain a value.
  constexpr optional() noexcept : m_value(nullptr) {}

  constexpr optional(nullopt_t) noexcept : m_value(nullptr) {}

  /// Copy constructor
  ///
  /// If `rhs` contains a value, the stored value is direct-initialized with
  /// it. Otherwise, the constructed optional is empty.
  TL_OPTIONAL_11_CONSTEXPR optional(const optional &rhs) noexcept = default;

  /// Move constructor
  ///
  /// If `rhs` contains a value, the stored value is direct-initialized with
  /// it. Otherwise, the constructed optional is empty.
  TL_OPTIONAL_11_CONSTEXPR optional(optional &&rhs) = default;

  /// Constructs the stored value with `u`.
  template <class U = T,
            detail::enable_if_t<!detail::is_optional<detail::decay_t<U>>::value>
                * = nullptr>
  constexpr optional(U &&u)  noexcept : m_value(std::addressof(u)) {
    static_assert(std::is_lvalue_reference<U>::value, "U must be an lvalue");
  }

  template <class U>
  constexpr explicit optional(const optional<U> &rhs) noexcept : optional(*rhs) {}

  /// No-op
  ~optional() = default;

  /// Assignment to empty.
  ///
  /// Destroys the current value if there is one.
  optional &operator=(nullopt_t) noexcept {
    m_value = nullptr;
    return *this;
  }

  /// Copy assignment.
  ///
  /// Rebinds this optional to the referee of `rhs` if there is one. Otherwise
  /// resets the stored value in `*this`.
  optional &operator=(const optional &rhs) = default;

  /// Rebinds this optional to `u`.
  template <class U = T,
            detail::enable_if_t<!detail::is_optional<detail::decay_t<U>>::value>
                * = nullptr>
  optional &operator=(U &&u) {
    static_assert(std::is_lvalue_reference<U>::value, "U must be an lvalue");
    m_value = std::addressof(u);
    return *this;
  }

  /// Converting copy assignment operator.
  ///
  /// Rebinds this optional to the referee of `rhs` if there is one. Otherwise
  /// resets the stored value in `*this`.
  template <class U> optional &operator=(const optional<U> &rhs) noexcept {
    m_value = std::addressof(rhs.value());
    return *this;
  }

  /// Rebinds this optional to `u`.
  template <class U = T,
            detail::enable_if_t<!detail::is_optional<detail::decay_t<U>>::value>
                * = nullptr>
  optional &emplace(U &&u) noexcept {
    return *this = std::forward<U>(u);
  }

  void swap(optional &rhs) noexcept { std::swap(m_value, rhs.m_value); }

  /// Returns a pointer to the stored value
  constexpr const T *operator->() const noexcept { return m_value; }

  TL_OPTIONAL_11_CONSTEXPR T *operator->() noexcept { return m_value; }

  /// Returns the stored value
  TL_OPTIONAL_11_CONSTEXPR T &operator*() noexcept { return *m_value; }

  constexpr const T &operator*() const noexcept { return *m_value; }

  constexpr bool has_value() const noexcept { return m_value != nullptr; }

  constexpr explicit operator bool() const noexcept {
    return m_value != nullptr;
  }

  /// Returns the contained value if there is one, otherwise throws bad_optional_access
  TL_OPTIONAL_11_CONSTEXPR T &value() {
    if (has_value())
      return *m_value;
    throw bad_optional_access();
  }
  TL_OPTIONAL_11_CONSTEXPR const T &value() const {
    if (has_value())
      return *m_value;
    throw bad_optional_access();
  }

  /// Returns the stored value if there is one, otherwise returns `u`
  template <class U> constexpr T value_or(U &&u) const & noexcept {
    static_assert(std::is_copy_constructible<T>::value &&
                      std::is_convertible<U &&, T>::value,
                  "T must be copy constructible and convertible from U");
    return has_value() ? **this : static_cast<T>(std::forward<U>(u));
  }

  /// \group value_or
  template <class U> TL_OPTIONAL_11_CONSTEXPR T value_or(U &&u) && noexcept {
    static_assert(std::is_move_constructible<T>::value &&
                      std::is_convertible<U &&, T>::value,
                  "T must be move constructible and convertible from U");
    return has_value() ? **this : static_cast<T>(std::forward<U>(u));
  }

  /// Destroys the stored value if one exists, making the optional empty
  void reset() noexcept { m_value = nullptr; }

private:
  T *m_value;
}; // namespace tl



} // namespace tl

namespace std {
// TODO SFINAE
template <class T> struct hash<tl::optional<T>> {
  ::std::size_t operator()(const tl::optional<T> &o) const {
    if (!o.has_value())
      return 0;

    return std::hash<tl::detail::remove_const_t<T>>()(*o);
  }
};
} // namespace std

#endif

#define RVR_FWD(x) static_cast<decltype(x)&&>(x)
#define RVR_RETURNS(e) -> decltype(e) { return e; }

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

    // count()
    // * Returns the number of elements in the river
    constexpr auto count() -> int
    {
        int i = 0;
        for_each([&](auto&&){ ++i; });
        return i;
    }

    // consume()
    // Draws from the river until it is empty
    constexpr void consume()
    {
        for_each([](auto&&){});
    }

    // collect<T>() and collect<Z>(): requires collect.hpp
    template <typename T> constexpr auto collect();
    template <template <typename...> class Z> constexpr auto collect();
    constexpr auto into_vec();
    constexpr auto into_str();


    ///////////////////////////////////////////////////////////////////
    // and a bunch of river adapters
    ///////////////////////////////////////////////////////////////////

    // ref(): requires ref.hpp
    constexpr auto ref() &;

    // map(f): requires map.hpp
    template <typename F> constexpr auto map(F&& f) &;
    template <typename F> constexpr auto map(F&& f) &&;

    // filter(p): requires filter.hpp
    template <typename P = std::identity> constexpr auto filter(P&& = {}) &;
    template <typename P = std::identity> constexpr auto filter(P&& = {}) &&;

    // chain(p...): requires chain.hpp
    template <River... R> constexpr auto chain(R&&...) &;
    template <River... R> constexpr auto chain(R&&...) &&;

    // take(n): requires take.hpp
    constexpr auto take(int n) &;
    constexpr auto take(int n) &&;

    // drop(n): requires drop.hpp
    constexpr auto drop(int n) &;
    constexpr auto drop(int n) &&;

    // split(e): requires split.hpp
    template <typename D=Derived> constexpr auto split(value_t<D>) &;
    template <typename D=Derived> constexpr auto split(value_t<D>) &&;
};

// and in non-member, callable form
#define RVR_ALGO_FUNCTION_OBJECT(name)                                 \
    inline constexpr auto name =                           \
        [](River auto&& r, auto&&... args)                 \
            RVR_RETURNS(RVR_FWD(r).name(RVR_FWD(args)...))

RVR_ALGO_FUNCTION_OBJECT(all);
RVR_ALGO_FUNCTION_OBJECT(any);
RVR_ALGO_FUNCTION_OBJECT(none);
RVR_ALGO_FUNCTION_OBJECT(for_each);
RVR_ALGO_FUNCTION_OBJECT(next);
RVR_ALGO_FUNCTION_OBJECT(next_ref);
RVR_ALGO_FUNCTION_OBJECT(fold);
RVR_ALGO_FUNCTION_OBJECT(sum);
RVR_ALGO_FUNCTION_OBJECT(product);
RVR_ALGO_FUNCTION_OBJECT(count);
RVR_ALGO_FUNCTION_OBJECT(consume);

}

#endif

#ifndef RIVERS_CHAIN_HPP
#define RIVERS_CHAIN_HPP


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// chain: takes N RiverOf<T>'s and produces a single RiverOf<T> that consumes
// them sequentially
////////////////////////////////////////////////////////////////////////////

template <River... Rs>
    requires requires {
        typename std::common_reference_t<reference_t<Rs>...>;
    }
struct Chain : RiverBase<Chain<Rs...>>
{
private:
    std::tuple<Rs...> bases;

public:
    using reference = std::common_reference_t<reference_t<Rs>...>;

    constexpr Chain(Rs... bases) : bases(std::move(bases)...) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return std::apply([&](Rs&... rs){
            return (rs.while_(pred) and ...);
        }, bases);
    }

    void reset() requires (ResettableRiver<Rs> and ...)
    {
        std::apply([&](Rs&... rs){
            (rs.reset(), ...);
        }, bases);
    }
};

struct {
    template <River... Rs>
    constexpr auto operator()(Rs&&... rs) const {
        return Chain(RVR_FWD(rs)...);
    }
} inline constexpr chain;

template <typename Derived>
template <River... Rs>
constexpr auto RiverBase<Derived>::chain(Rs&&... rs) & {
    return Chain(self(), RVR_FWD(rs)...);
}

template <typename Derived>
template <River... Rs>
constexpr auto RiverBase<Derived>::chain(Rs&&... rs) && {
    return Chain(RVR_FWD(self()), RVR_FWD(rs)...);
}

}

#endif
#ifndef RIVERS_COLLECT_HPP
#define RIVERS_COLLECT_HPP

#ifndef RIVERS_TAG_INVOKE_HPP
#define RIVERS_TAG_INVOKE_HPP

namespace rvr {

namespace detail {
    // void tag_invoke() = delete;

    struct tag_invoke_fn {
        template <typename Tag, typename... Args>
            requires requires (Tag tag, Args&&... args) {
                tag_invoke((Tag&&)tag, (Args&&)args...);
            }
        constexpr auto operator()(Tag tag, Args&&... args) const
            -> decltype(auto)
        {
            return tag_invoke((Tag&&)tag, (Args&&)args...);
        }
    };
}

inline namespace rvr_cpo {
    inline constexpr detail::tag_invoke_fn tag_invoke;
}

template <typename Tag, typename... Args>
concept tag_invocable = requires (Tag tag, Args&&... args) {
    ::rvr::tag_invoke((Tag&&)tag, (Args&&)args...);
};

}

#endif
#include <vector>
#include <string>

namespace rvr {

template <typename T>
struct collect_fn {
    template <River R>
        requires std::ranges::input_range<T>
    friend constexpr auto tag_invoke(collect_fn<T>, R&& r) -> T {
        T output;
        r.for_each([&](auto&& elem){
            output.push_back(RVR_FWD(elem));
        });
        return output;
    }

    template <River R>
        requires tag_invocable<collect_fn, R>
    constexpr auto operator()(R&& r) const {
        return rvr::tag_invoke(*this, RVR_FWD(r));
    }
};

template <typename T>
inline constexpr collect_fn<T> collect;


template <typename Derived>
template <typename T>
constexpr auto RiverBase<Derived>::collect() {
    return rvr::collect<T>(self());
}

template <typename Derived>
template <template <typename...> class Z>
constexpr auto RiverBase<Derived>::collect() {
    struct iterator {
        using value_type = value_t<Derived>;
        using reference = reference_t<Derived>;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        auto operator*() const -> reference;
        auto operator++() -> iterator&;
        auto operator++(int) -> iterator;
        auto operator==(iterator const&) const -> bool;
    };
    static_assert(std::input_iterator<iterator>);
    using T = decltype(Z(std::declval<iterator>(), std::declval<iterator>()));
    return collect<T>();
}

template <typename Derived>
constexpr auto RiverBase<Derived>::into_vec() {
    return collect<std::vector>();
}

template <typename Derived>
constexpr auto RiverBase<Derived>::into_str() {
    return collect<std::string>();
}

RVR_ALGO_FUNCTION_OBJECT(into_vec);
RVR_ALGO_FUNCTION_OBJECT(into_str);


}

#endif
#ifndef RIVERS_DROP_HPP
#define RIVERS_DROP_HPP


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// drop: takes a (RiverOf<T> r, int n) and produces a new RiverOf<T> which
// includes all but the first n elements of r (or no elements,
// if r doesn't have n elements)
////////////////////////////////////////////////////////////////////////////

template <River R>
struct Drop : RiverBase<Drop<R>>
{
private:
    R base;
    int n;
    int i = 0;

public:
    using reference = reference_t<R>;

    constexpr Drop(R base, int n) : base(std::move(base)), n(n) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return base.while_([&](reference elem){
            if (i++ < n) {
                return true;
            } else {
                return std::invoke(pred, elem);
            }
        });
    }

    void reset() requires ResettableRiver<R>
    {
        base.reset();
        i = 0;
    }
};

struct {
    template <River R>
    constexpr auto operator()(R&& r, int n) const {
        return Drop(RVR_FWD(r), n);
    }
} inline constexpr drop;

template <typename Derived>
constexpr auto RiverBase<Derived>::drop(int n) & {
    return Drop(self(), n);
}

template <typename Derived>
constexpr auto RiverBase<Derived>::drop(int n) && {
    return Drop(RVR_FWD(self()), n);
}

}

#endif
#ifndef RIVERS_FILTER_HPP
#define RIVERS_FILTER_HPP


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// filter: takes a (RiverOf<T>, T -> bool) and produces a new RiverOf<T>
// containing only the elements that satisfy the predicate
////////////////////////////////////////////////////////////////////////////
template <River R, typename P>
    requires std::predicate<P&, reference_t<R>>
struct Filter : RiverBase<Filter<R, P>> {
private:
    R base;
    P filter;

public:
    using reference = reference_t<R>;

    constexpr Filter(R base, P pred) : base(std::move(base)), filter(std::move(pred)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return base.while_([&](reference e){
            if (std::invoke(filter, e)) {
                return std::invoke(pred, RVR_FWD(e));
            } else {
                // if we don't satisfy our internal predicate, we keep going
                return true;
            }
        });
    }

    void reset() requires ResettableRiver<R> {
        base.reset();
    }
};

struct {
    template <River R, typename P = std::identity>
        requires std::predicate<P&, reference_t<R>&>
    constexpr auto operator()(R&& base, P&& pred = {}) const {
        return Filter(RVR_FWD(base), RVR_FWD(pred));
    }
} inline constexpr filter;

template <typename Derived>
template <typename P>
constexpr auto RiverBase<Derived>::filter(P&& pred) & {
    static_assert(std::predicate<P&, reference_t<Derived>&>);
    return Filter(self(), RVR_FWD(pred));
}

template <typename Derived>
template <typename P>
constexpr auto RiverBase<Derived>::filter(P&& pred) && {
    static_assert(std::predicate<P&, reference_t<Derived>&>);
    return Filter(RVR_FWD(self()), RVR_FWD(pred));
}

}

#endif
#ifndef RIVERS_FROM_HPP
#define RIVERS_FROM_HPP

#include <ranges>


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// Converting a C++ Range to a River
// * from(r)           for a range
// * from(first, last) for an iterator/sentinel pair
// The resulting river is resettable if the source range is forward or better
////////////////////////////////////////////////////////////////////////////
template <std::ranges::input_range R>
struct From : RiverBase<From<R>>
{
private:
    R base;
    std::ranges::iterator_t<R> it = std::ranges::begin(base);
    std::ranges::sentinel_t<R> end = std::ranges::end(base);

public:
    using reference = std::ranges::range_reference_t<R>;
    using value_type = std::ranges::range_value_t<R>;

    constexpr From(R&& r) : base(std::move(r)) { }

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

struct from_fn {
    template <std::ranges::input_range R>
    friend constexpr auto tag_invoke(from_fn, R&& r) {
        using U = std::remove_cvref_t<R>;
        if constexpr (std::ranges::view<R>) {
            return From<U>(RVR_FWD(r));
        } else if constexpr (std::is_lvalue_reference_v<R>) {
            return From(std::ranges::ref_view(r));
        } else {
            return From(RVR_FWD(r));
        }
    }

    template <typename R>
        requires tag_invocable<from_fn, R>
    constexpr auto operator()(R&& r) const {
        return rvr::tag_invoke(*this, RVR_FWD(r));
    }

    template <std::input_iterator I, std::sentinel_for<I> S>
    constexpr auto operator()(I first, S last) const {
        return From(std::ranges::subrange(std::move(first), std::move(last)));
    }
} inline constexpr from;

}

#endif
#ifndef RIVERS_MAP_HPP
#define RIVERS_MAP_HPP


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// map: takes a (RiverOf<T>, T -> U) and produces a RiverOf<U>
////////////////////////////////////////////////////////////////////////////
template <River R, typename F>
    requires std::regular_invocable<F&, reference_t<R>>
struct Map : RiverBase<Map<R, F>> {
private:
    R base;
    F f;

public:
    using reference = std::invoke_result_t<F&, reference_t<R>>;

    constexpr Map(R base, F f) : base(std::move(base)), f(std::move(f)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return base.while_([&](reference_t<R> e){
            return std::invoke(pred, std::invoke(f, RVR_FWD(e)));
        });
    }

    void reset() requires ResettableRiver<R> {
        base.reset();
    }
};

struct {
    template <River R, typename F>
        requires std::regular_invocable<F&, reference_t<R>>
    constexpr auto operator()(R&& base, F&& f) const {
        return Map(RVR_FWD(base), RVR_FWD(f));
    }
} inline constexpr map;

template <typename Derived>
template <typename F>
constexpr auto RiverBase<Derived>::map(F&& f) & {
    static_assert(std::invocable<F&, reference_t<Derived>>);
    return Map(self(), RVR_FWD(f));
}

template <typename Derived>
template <typename F>
constexpr auto RiverBase<Derived>::map(F&& f) && {
    static_assert(std::invocable<F&, reference_t<Derived>>);
    return Map(RVR_FWD(self()), RVR_FWD(f));
}

}

#endif
#ifndef RIVERS_OF_HPP
#define RIVERS_OF_HPP


////////////////////////////////////////////////////////////////////////////
// Convert specifically provided values to a River
// * from_values({1, 2, 3})
// * from_values(4, 5, 6)
// In each case, a vector is created from the arguments and that vector is
// turned into a River, via from_cpp
////////////////////////////////////////////////////////////////////////////

namespace rvr {

template <typename T>
struct Single : RiverBase<Single<T>> {
private:
    T value;
    bool consumed = false;

public:
    using reference = T&;

    template <typename U>
    explicit constexpr Single(U&& value) : value(RVR_FWD(value)) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        if (not consumed) {
            RVR_SCOPE_EXIT { consumed = true; };
            return std::invoke(pred, value);
        } else {
            return false;
        }
    }

    void reset() {
        consumed = false;
    }
};

struct {
    template <typename T>
    constexpr auto operator()(std::initializer_list<T> values) const {
        return from(std::vector(values.begin(), values.end()));
    }

    template <typename... Ts>
    constexpr auto operator()(Ts&&... values) const {
        return from(std::vector{values...});
    }

    // special case a single element
    template <typename T>
    constexpr auto operator()(T&& value) const {
        return Single<std::decay_t<T>>(RVR_FWD(value));
    }
} inline constexpr of;

}

#endif
#ifndef RIVERS_REF_HPP
#define RIVERS_REF_HPP


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// ref: takes a RiverOf<T> and returns a new RiverOf<T> that is a reference
// to the original (similar to ranges::ref_view)
////////////////////////////////////////////////////////////////////////////

template <River R>
struct Ref : RiverBase<Ref<R>>
{
private:
    R* base;

public:
    using reference = reference_t<R>;

    constexpr Ref(R& base) : base(&base) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        return base->while_(RVR_FWD(pred));
    }

    void reset() requires ResettableRiver<R> {
        base->reset();
    }
};

struct {
    template <River R>
    constexpr auto operator()(R& r) const {
        return Ref(r);
    }
} inline constexpr ref;

template <typename Derived>
constexpr auto RiverBase<Derived>::ref() & {
    return Ref(self());
}


}

#endif
#ifndef RIVERS_SEQ_HPP
#define RIVERS_SEQ_HPP


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// Python-style range.
// seq(3, 5) includes the elements [3, 4]
// seq(5) includes the elements [0, 1, 2, 3, 4]
// Like std::views::iota, except that seq(e) are the elements in [E(), e)
// rather than the  being the elements in [e, inf)
////////////////////////////////////////////////////////////////////////////
template <std::weakly_incrementable I>
    requires std::equality_comparable<I>
struct Seq : RiverBase<Seq<I>>
{
private:
    I from = I();
    I to;
    I orig = from;

public:
    using reference = I;

    explicit constexpr Seq(I from, I to) : from(from), to(to) { }
    explicit constexpr Seq(I to) requires std::default_initializable<I> : to(to) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        while (from != to) {
            RVR_SCOPE_EXIT { ++from; };
            if (not std::invoke(pred, I(from))) {
                return false;
            }
        }
        return true;
    }

    void reset() {
        from = orig;
    }
};

struct {
    template <std::weakly_incrementable I> requires std::equality_comparable<I>
    constexpr auto operator()(I from, I to) const {
        return Seq(std::move(from), std::move(to));
    }

    template <std::weakly_incrementable I>
        requires std::equality_comparable<I>
                && std::default_initializable<I>
    constexpr auto operator()(I to) const {
        return Seq(std::move(to));
    }
} inline constexpr seq;

}

#endif
#ifndef RIVERS_SPLIT_HPP
#define RIVERS_SPLIT_HPP


namespace rvr {

///////////////////////////////////////////////////////////////////////////
// split: takes a (RiverOf<T> r, T v) and produces a new RiverOf<RiverOf<T>>
// that is ... split... on ever instance of v
////////////////////////////////////////////////////////////////////////////

template <River R>
struct Split : RiverBase<Split<R>>
{
private:
    struct Inner : RiverBase<Inner>
    {
    private:
        friend Split;
        Split* parent;
        bool found_delim = false;

    public:
        using reference = reference_t<R>;
        explicit Inner(Split* p) : parent(p) { }

        constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool;
    };

    R base;
    value_t<R> delim;
    bool exhausted = false;
    bool partial = false;
    Inner inner;

public:
    using reference = Ref<Inner>;

    constexpr Split(R base, value_t<R> delim)
        : base(std::move(base))
        , delim(std::move(delim))
        , inner(this)
    { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        if (exhausted) {
            return true;
        }

        if (partial) {
            inner.consume();
            if (exhausted) {
                return true;
            }
            partial = false;
        }

        for (;;) {
            inner = Inner(this);
            if (std::invoke(pred, inner.ref())) {
                inner.consume();
                if (exhausted) {
                    return true;
                }
            } else {
                partial = not inner.found_delim;
                return false;
            }
        }
    }

    void reset() requires ResettableRiver<R> {
        base.reset();
        exhausted = false;
    }
};

template <River R>
constexpr auto Split<R>::Inner::while_(PredicateFor<reference> auto&& pred) -> bool {
    if (found_delim) {
        return true;
    }

    bool result = parent->base.while_([&](reference elem){
        if (elem == parent->delim) {
            found_delim = true;
            return false;
        } else {
            return std::invoke(pred, elem);
        }
    });

    if (result and not found_delim) {
        parent->exhausted = true;
    }
    return result;
}

struct {
    template <River R>
    constexpr auto operator()(R&& r, value_t<R> delim) const {
        return Split(RVR_FWD(r), std::move(delim));
    }
} inline constexpr split;

template <typename Derived>
template <typename D>
constexpr auto RiverBase<Derived>::split(value_t<D> delim) & {
    return Split(self(), std::move(delim));
}

template <typename Derived>
template <typename D>
constexpr auto RiverBase<Derived>::split(value_t<D> delim) && {
    return Split(RVR_FWD(self()), std::move(delim));
}


}

#endif
#ifndef RIVERS_TAKE_HPP
#define RIVERS_TAKE_HPP


namespace rvr {

////////////////////////////////////////////////////////////////////////////
// take: takes a (RiverOf<T> r, int n) and produces a new RiverOf<T> which
// includes up to the first n elements of r (or fewer, if r doesn't have n elements)
////////////////////////////////////////////////////////////////////////////

template <River R>
struct Take : RiverBase<Take<R>>
{
private:
    R base;
    int n;
    int i = 0;

public:
    using reference = reference_t<R>;

    constexpr Take(R base, int n) : base(std::move(base)), n(n) { }

    constexpr auto while_(PredicateFor<reference> auto&& pred) -> bool {
        if (i != n) {
            return base.while_([&](reference elem){
                ++i;
                return std::invoke(pred, elem) and i != n;
            }) or i == n;
        } else {
            return true;
        }
    }

    void reset() requires ResettableRiver<R>
    {
        base.reset();
        i = 0;
    }
};

struct {
    template <River R>
    constexpr auto operator()(R&& r, int n) const {
        return Take(RVR_FWD(r), n);
    }
} inline constexpr take;

template <typename Derived>
constexpr auto RiverBase<Derived>::take(int n) & {
    return Take(self(), n);
}

template <typename Derived>
constexpr auto RiverBase<Derived>::take(int n) && {
    return Take(RVR_FWD(self()), n);
}

}

#endif

#ifndef RIVERS_FORMAT_HPP
#define RIVERS_FORMAT_HPP

#if __has_include(<fmt/format.h>)
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace rvr {

template <typename T>
struct wrapped_debug_formatter : fmt::formatter<T> {
    bool use_debug = false;

    constexpr void format_as_debug() {
        use_debug = true;
    }

    template <typename FormatContext>
    constexpr auto format(T const& value, FormatContext& ctx) const {
        if constexpr (fmt::detail::is_std_string_like<T>::value or std::is_same_v<T, char>) {
            if (use_debug) {
                return fmt::detail::write_range_entry<char>(ctx.out(), value);
            }
        }

        return fmt::formatter<T>::format(value, ctx);
    }
};

template <typename V>
struct river_formatter {
    fmt::detail::dynamic_format_specs<char> specs;
    wrapped_debug_formatter<V> underlying;
    bool is_debug = false;
    bool use_brackets = true;

    template <typename Iterator, typename Handler>
    constexpr auto parse_range_specs(Iterator begin, Iterator end, Handler&& handler) {
        // parse_align isn't quite right because we need to escape the
        // : if that's the desired fill
        if (*begin == '\\') {
            FMT_ASSERT(begin[1] == ':', "expected colon");
            begin = fmt::detail::parse_align(begin + 1, end, handler);
            FMT_ASSERT(specs.align != fmt::align::none, "escape but no align?");
            end = std::ranges::find(begin, end, ':');
        } else {
            end = std::ranges::find(begin, end, ':');
            if (begin == end) return begin;
            begin = fmt::detail::parse_align(begin, end, handler);
        }

        if (begin == end) return end;

        begin = fmt::detail::parse_width(begin, end, handler);
        if (begin == end) return begin;

        if (*begin == '?') {
            is_debug = true;
            ++begin;
            if (begin == end) return begin;
        }

        if (begin != end && *begin != '}') {
            switch (*begin++) {
            case 's':
                specs.type = fmt::presentation_type::string;
                break;
            case 'e':
                use_brackets = false;
                break;
            default:
                throw fmt::format_error("unknown type");
            }
        }

        return begin;
    }

    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        auto begin = ctx.begin();
        auto end = ctx.end();
        if (begin == end || *begin == '}') {
            if constexpr (requires { underlying.format_as_debug(); }) {
                underlying.format_as_debug();
            }
            return begin;
        }

        using handler_type = fmt::detail::dynamic_specs_handler<ParseContext>;
        auto checker =
            fmt::detail::specs_checker<handler_type>(handler_type(specs, ctx), fmt::detail::type::none_type);
        auto it = parse_range_specs(ctx.begin(), ctx.end(), checker);

        if (it == ctx.end() || *it == '}') {
            if constexpr (requires { underlying.format_as_debug(); }) {
                underlying.format_as_debug();
            }
        } else {
            ++it;
        }

        ctx.advance_to(it);
        return underlying.parse(ctx);
    }

    template <typename R, typename FormatContext>
        requires std::same_as<std::remove_cvref_t<reference_t<R>>, V>
    constexpr auto format(R&& r, FormatContext& ctx) const -> typename FormatContext::iterator
    {
        if (specs.type == fmt::presentation_type::string) {
            if constexpr (std::same_as<V, char>) {
                auto s = r.into_str();
                if (is_debug) {
                    return fmt::detail::write_range_entry<char>(ctx.out(), s);
                } else {
                    return fmt::detail::write(ctx.out(), fmt::string_view(s), specs);
                }
            } else {
                throw fmt::format_error("wat");
            }
        }

        fmt::memory_buffer buf;
        fmt::basic_format_context<fmt::appender, char>
            bctx(fmt::appender(buf), ctx.args(), ctx.locale());

        auto out = bctx.out();
        if (use_brackets) {
            *out++ = '[';
        }

        bool first = true;
        r.for_each([&](auto&& elem){
            if (not first) {
                *out++ = ',';
                *out++ = ' ';
            } else {
                first = false;
            }

            // have to format every element via the underlying formatter
            bctx.advance_to(std::move(out));
            out = underlying.format(elem, bctx);
        });

        if (use_brackets) {
            *out++ = ']';
        }

        return fmt::detail::write(ctx.out(), fmt::string_view(buf.data(), buf.size()), specs);
    }
};

}

template <rvr::River R>
struct fmt::formatter<R>
    : rvr::river_formatter<std::remove_cvref_t<rvr::reference_t<R>>>
{
    template <typename FormatContext>
    constexpr auto format(R& r, FormatContext& ctx) {
        return rvr::river_formatter<std::remove_cvref_t<rvr::reference_t<R>>>::format(r, ctx);
    }
};

#endif

#endif

#endif
