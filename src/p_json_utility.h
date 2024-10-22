#pragma once
#include "p_json_error.h"
#include <iostream>
#include <functional>
#include <string>
#include <tuple>


namespace mini_json
{

using std::make_tuple;

/* prop("name") -> BaseProperty{"name"}
 *
 * prop("name")[&Class::name] -> ByMemberProperty<BaseProperty>{"name", &name} // Complete
 * prop("name")[&Class::get_name] -> MemberGetterProperty<BaseProperty>{"name", &get_name} // Needed
 *
 * prop("name")[&Class::name](&Class::set_name) -> MemberSetterProperty<ByMemberProperty<BaseProperty>>{"name", &name, &set_name} // Complete
 * prop("name")[&Class::name](&Class::name) -> MemberSetterProperty<ByMemberProperty<BaseProperty>>{"name", &name, &set_name} // Error :)
 * prop("name")[&Class::get_name](&Class::set_name) -> MemberSetterProperty<MemberGetterProperty<BaseProperty>>{"name", &get_name, &set_name} // Complete
 * prop("name")[&Class::get_name](&Class::name) -> ByMemberProperty<MemberGetterProperty<BaseProperty>>{"name", &get_name, &set_name} // Error :)
 *
 *
 *
 */
struct PropertyLvl1
{
    const char* name;

    template<typename C, typename T, typename G, bool _by_member = false>
    struct PropertyLvl2
    {
        using Class = C;
        using Type = T;
        constexpr static bool by_member = _by_member;


        template<typename S>
        struct PropertyLvl3
        {
            using Class = C;
            using Type = T;

            const char* name;
            // std::function<const T&(const Class*)>
            G get;
            // std::function<void(Class*, T&&)>
            S set;

            bool is_required = false;

            PropertyLvl2&& required() && { is_required = true; return std::move(*this); }
            PropertyLvl2&& operator~() && { return std::move(required()); }
        };


        const char* name;

        // std::function<const T&(const Class*)>
        G get;

        T C::* _member = nullptr;

        bool is_required = false;

        template<typename Ty = T, typename std::enable_if<std::is_function_v<Ty>, bool>::type = true>
        constexpr auto operator()(Ty C::* set) &&
        {
            auto l = [set=set](C* cls, T&& value) { return (cls->*set)(std::move(value)); };
            return PropertyLvl3<decltype(l)>{name, get, l};
        }

        void set(Class* cls, Type&& value)
        {
            if (not by_member)
                throw std::runtime_error("Trying to call set by not member property " + std::string(name));
            cls->*_member = std::move(value);
        }

        PropertyLvl2&& required() && { is_required = true; return std::move(*this); }
        PropertyLvl2&& operator~() && { return std::move(required()); }

    };

    template<typename C, typename T, typename std::enable_if<not std::is_function_v<T>, bool>::type = true>
    constexpr auto operator[](T C::* member) &&
    {
        auto l = [member](const C* cls) -> const T& { return cls->*member; };
        return PropertyLvl2<C, T, decltype(l), true>{name, l, member};
    }

    template<typename C, typename T, typename std::enable_if<std::is_function_v<T>, bool>::type = true>
    constexpr auto operator[](T C::* get) &&
    {
        using ty = decltype((std::declval<C*>()->*get)());
        auto l = [get](const C* cls) { return (cls->*get)(); };
        return PropertyLvl2<C, std::decay_t<ty>, decltype(l)>{name, l};
    }


    template<typename T>
    struct PropertyValue
    {
        using Class = void;
        using Type = std::conditional_t<std::is_same_v<T, const char*>, std::string, T>;

        T value;

        const T& get(const void* cls)
        {
            return value;
        }

        void set(void* cls, Type&& value)
        {
            if (this->value != value)
                throw ParseError("Value assertation error");
        }

    };

    template<typename T>
    constexpr PropertyValue<T> operator=(T&& value)
    {
        return PropertyValue<T>{std::move(value)};
    }

};


template <typename T>
class IsJsonParseble
{
    using Yes = char;
    struct No
    {
        char _[2];
    };

    template <typename C> static Yes test(decltype(&C::json_properties));
    static No test(...);

public:
    constexpr static bool value = sizeof(test<T>(nullptr)) == sizeof(Yes);
};

template<typename T>
struct SerializeInfo
{
    static_assert(IsJsonParseble<T>::value,
                  "Type must specify 'json_properties' static member "
                  "function to be used in this context!");

    template<typename Prop>
    static constexpr Prop cvt(Prop prop)
    {
        return prop;
    }

    template<size_t... indices>
    static constexpr auto convert(std::index_sequence<indices...>)
    {
        constexpr auto props = T::json_properties();
        return make_tuple(cvt(std::get<indices>(props))...);
    }

    static constexpr auto fields()
    {
        constexpr auto n_properties = std::tuple_size<decltype(T::json_properties())>::value;
        return convert(std::make_index_sequence<n_properties>());
    }

};
}

namespace mini_json::_private
{
template <typename C, typename T>
struct PropertyImpl
{
    constexpr PropertyImpl(const char* aName)
        : name{aName}
    {
    }

    using Class = C;
    using Type = T;

    void write(Class* cls, Type&& value);
    const Type& read(const Class* cls);

    const char* name;
};

template <typename T>
struct Type
{
};

template <typename T, T... S, typename F>
constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f)
{
    using unpack_t = int[];
    (void)unpack_t{(static_cast<void>(f(std::integral_constant<T, S>{})), 0)..., 0};
}

constexpr auto str_equal(const char* lhs, const char* rhs)
{
    for (; *lhs && *rhs; ++lhs, ++rhs)
    {
        if (*lhs != *rhs)
            return false;
    }
    return *lhs == *rhs;
}

template <typename T, typename Fun>
constexpr void executeByPropertyName(const char* name, Fun&& f)
{
    constexpr auto n_properties = std::tuple_size<decltype(T::json_properties())>::value;
    auto found = false;
    for_sequence(std::make_index_sequence<n_properties>{}, [&](auto i) {
        constexpr auto property = std::get<i>(T::json_properties());
        if (str_equal(property.name, name))
        {
            found = true;
            f(property);
        }
    });
    if (!found)
    {
        throw UnexpectedPropertyName(name);
    }
}

template <typename T>
class IsJsonParseble
{
    using Yes = char;
    struct No
    {
        char _[2];
    };

    template <typename C> static Yes test(decltype(&C::json_properties));
    static No test(...);

public:
    constexpr static bool value = sizeof(test<T>(nullptr)) == sizeof(Yes);
};
} // namespace mini_json::_private

