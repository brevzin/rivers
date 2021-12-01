#include "catch.hpp"

namespace Catch {
    template <typename T>
    struct StringMaker<tl::optional<T>> {
        static auto convert(tl::optional<T> const& value) -> std::string {
            ReusableStringStream rs;
            if (value) {
                rs << "Some(" << Detail::stringify(*value) << ')';
            } else {
                rs << "None";
            }
            return rs.str();
        }
    };
}

template <typename T>
inline auto Some(T&& t) -> tl::optional<std::remove_cvref_t<T>> {
    return RVR_FWD(t);
}
