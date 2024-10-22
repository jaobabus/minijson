#pragma once
#include "p_json_parser.h"
#include "p_json_serializer.h"
#include <functional>
#include <iostream>
#include <iterator>
#include <string>

namespace mini_json
{
template <typename C, typename T>
constexpr auto property(T C::*member, const char* name)
{
    struct PropertyMemberImpl
    {
        constexpr PropertyMemberImpl(T C::*aMember, const char* aName)
            : name{aName},
              member{aMember}
        {
        }

        using Class = C;
        using Type = T;

        void set(Class* cls, Type&& value) const { cls->*member = std::move(value); }
        const Type& get(const Class* cls) const { return cls->*member; }

        const char* name;
        T Class::*member;
    };
    return PropertyMemberImpl{member, name};
}

template <typename TClass, typename T, typename Setter, typename Getter>
constexpr auto property_common(Setter setter, Getter getter, const char* name)
{

    struct PropertyWGSImpl
    {
        constexpr PropertyWGSImpl(Setter setter, Getter getter, const char* aName)
            : name{aName},
              setter{setter},
              getter{getter}
        {
        }

        using Class = TClass;
        using Type = T;

        void set(Class* cls, T&& value) const { std::invoke(setter, cls, std::move(value)); }
        const T& get(const Class* cls) const { return std::invoke(getter, cls); }

        const char* name;
        Setter setter;
        Getter getter;
    };
    return PropertyWGSImpl{setter, getter, name};
}


template <typename Class, typename T, typename Setter>
constexpr auto property(Setter setter, const T& (Class::* getter)() const, const char* name)
{
    return property_common<Class, T>(setter, getter, name);
}

template <typename Class, typename T, typename Getter>
constexpr auto property(void (Class::* setter)(T&&), Getter getter, const char* name)
{
    return property_common<Class, T>(setter, getter, name);
}

template <typename Class, typename T>
constexpr auto property(void (Class::* setter)(T&&), const T& (Class::* getter)() const, const char* name)
{
    return property_common<Class, T>(setter, getter, name);
}

template <typename Class, typename T>
constexpr auto property(void (Class::* setter)(T&&), const T Class::* member, const char* name)
{
    return property_common<Class, T>(setter, [=](const Class* cls) -> const T& { return cls->*member; }, name);
}

template <typename Class, typename T, typename Setter>
constexpr auto property(Setter setter, const T Class::* member, const char* name)
{
    return property_common<Class, T>(setter, [=](const Class* cls) -> const T& { return cls->*member; }, name);
}



inline constexpr PropertyLvl1 prop(const char* name)
{
    return PropertyLvl1{name};
}


/**
     Parse value T
     T has to be default contructable
     and type T must have a static member function named 'json_properties'
     that returns a tuple of the the json properties to be parsed.
     Parsed properties must be able to be set by the parse method
     (declare them as public)
     */
template <typename T, typename FwIt>
T parse(FwIt begin, FwIt end)
{
    static_assert(_private::IsJsonParseble<T>::value,
                  "Type must specify 'json_properties' static member "
                  "function to be used in this context!");
    auto parser = _private::ParseImpl<FwIt>{begin, end};
    return parser.template parse<T>(_private::Type<T>{});
}

template <typename T> T
parse(std::istream& stream)
{
    return parse<T>(std::istream_iterator<char>(stream), std::istream_iterator<char>());
}

/**
     Serialize value T
     and type T must have a static member function named 'json_properties'
     that returns a tuple of the the json properties to be serialized.
     Serialized properties must be able to be read by the parse method
     (declare them as public)
     */
template <typename T, typename OStream>
void serialize(T const& item, OStream& result)
{
    auto serializer = _private::SerializerImpl<OStream>(result);
    serializer.serialize(item);
}
} // namespace mini_json

