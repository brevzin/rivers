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
