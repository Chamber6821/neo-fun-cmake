#pragma once

#pragma once

#include "./addressof.hpp"
#include "./attrib.hpp"
#include "./concepts.hpp"
#include "./fwd.hpp"
#include "./like.hpp"

#include <concepts>
#include <utility>

namespace neo {

namespace _obox_detail {

template <typename T>
struct box_base {
    /// The exact type that was given for the scalar_box
    using boxed_type = T;

    /// Whether the box contains a scalar type
    static constexpr inline bool contains_scalar = scalar_type<T>;
    /// Whether the box contains an object type
    static constexpr inline bool contains_object = object_type<T>;
    /// Whether the box wraps a reference
    static constexpr inline bool contains_ref = reference_type<T>;
    /// Whether the box wraps a void type
    static constexpr inline bool contains_void = void_type<T>;
};

}  // namespace _obox_detail

/**
 * @brief Defines a scalar type that contains a (possibly non-scalar) type.
 *
 * @tparam T Any scalar type, reference type, void type, bounded array type,
 * or any cv-qualified type thereof.
 *
 * For object types, inherits the construct/assign requirements of the wrapped
 * type.
 *
 * For references, is trivially copyable, but not default-constructible.
 *
 * For void, is empty and semiregular.
 */
template <typename T>
class scalar_box;

template <object_type T>
class scalar_box<T> : public _obox_detail::box_base<T> {
    // The contained object
    NEO_NO_UNIQUE_ADDRESS T _value;

    // Tag to construct from an array
    struct _array_construct {};

    // In-place construct an array member from a given array
    template <typename Arr, std::size_t... Idx>
    constexpr scalar_box(_array_construct, Arr&& arr, std::index_sequence<Idx...>) noexcept
        : _value{neo::forward_like<Arr>(arr[Idx])...} {}

public:
    scalar_box() = default;

    // clang-format off
    template <unalike<scalar_box> U>
        requires explicit_convertible_to<U&&, T>
        explicit(not implicit_convertible_to<U&&, T>)
    constexpr scalar_box(U&& arg)
        noexcept(noexcept(T(NEO_FWD(arg))))
        : _value(NEO_FWD(arg)) {}

    template <typename Elem, std::size_t Len>
    requires array_type<T>
         and convertible_to<Elem, remove_all_extents_t<T>>
    constexpr scalar_box(Elem (&&arr)[Len]) noexcept
        : scalar_box(_array_construct{}, NEO_FWD(arr), std::make_index_sequence<Len>{}) {}

    template <typename Elem, std::size_t Len>
    requires array_type<T>
         and convertible_to<Elem, remove_all_extents_t<T>>
    constexpr scalar_box(Elem (&arr)[Len]) noexcept
        : scalar_box(_array_construct{}, NEO_FWD(arr), std::make_index_sequence<Len>{}) {}

    /// Obtain a reference to the boxed value.
    [[nodiscard]] constexpr T&        get() & noexcept { return _value; }
    [[nodiscard]] constexpr const T&  get() const& noexcept { return _value; }
    [[nodiscard]] constexpr T&&       get() && noexcept { return NEO_MOVE(_value); }
    [[nodiscard]] constexpr const T&& get() const&& noexcept { return NEO_MOVE(_value); }

    constexpr T*       operator->() noexcept { return neo::addressof(_value); }
    constexpr const T* operator->() const noexcept { return neo::addressof(_value); }

    constexpr void friend do_repr(auto out, const scalar_box* self) noexcept {
        out.type("[boxed {}]", out.template repr_type<T>("?"));
        if (self) {
            out.value("{}", out.repr_value(self->get(), "?"));
        }
    }
};

template <reference_type Ref>
class scalar_box<Ref> : public _obox_detail::box_base<Ref> {
    using Pointer = add_pointer_t<Ref>;
    Pointer _ptr;

public:
    constexpr scalar_box() = delete;

    template <typename U>
    explicit constexpr scalar_box(U&& arg) noexcept requires requires(void (*fn)(Pointer)) {
        fn(neo::addressof(arg));
    } : _ptr(neo::addressof(arg)) {
    }

    /// Obtain the boxed reference
    [[nodiscard]] constexpr Ref& get() const noexcept { return *_ptr; }

    constexpr Pointer operator->() const noexcept { return _ptr; }

    constexpr void friend do_repr(auto out, const scalar_box* self) noexcept {
        out.type("[boxed {}]", out.template repr_type<Ref>("?&"));
        if (self) {
            out.value("{}", out.repr_value(self->get(), "?"));
        }
    }
};

template <void_type Void>
class scalar_box<Void> : public _obox_detail::box_base<Void> {
public:
    constexpr remove_cv_t<Void> get() const noexcept {}

    constexpr void friend do_repr(auto out, const scalar_box* self) noexcept {
        out.type("[boxed {}]", out.template repr_type<Void>());
        if (self) {
            out.value("(void)");
        }
    }
};

template <typename T>
scalar_box(T&& arg) -> scalar_box<T>;

}  // namespace neo
