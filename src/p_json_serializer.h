#pragma once
#include "p_json_error.h"
#include "p_json_utility.h"
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace mini_json::_private
{
template <typename TStream>
class SerializerImpl
{
    TStream& stream;

private:
    template<typename  T>
    static constexpr auto get_props()
    {
        return SerializeInfo<T>::fields();
    }

public:
    SerializerImpl(TStream& stream)
        : stream(stream)
    {
    }

    template <typename T> void serialize(T const& item)
    {
        stream << "{";

        const char* separator = "";

        constexpr auto props = get_props<T>();
        constexpr auto n_properties = std::tuple_size<decltype(props)>::value;
        for_sequence(std::make_index_sequence<n_properties>{}, [&](auto i)
        {
            constexpr auto prop = std::get<i>(props);
            stream << separator;
            this->serialize(std::string{prop.name});
            stream << ":";
            this->serialize(prop.get(&item));
            separator = ",";
        });

        stream << "}";
    }

    template <typename T> void serialize(std::vector<T> const& items)
    {
        stream << "[";
        const char* separator = "";
        for (auto& item : items)
        {
            stream << separator;
            separator = ",";
            this->serialize(item);
        }
        stream << "]";
    }

    template <typename... T> void serialize(std::variant<T...> const& item)
    {
        std::visit([this](auto& v) { this->serialize(v); }, item);
    }

    template <typename T> void serialize(std::optional<T> const& item)
    {
        if (item.has_value())
            this->serialize(item.value());
    }

    void serialize(std::string const& item)
    {
        stream << std::quoted(item);
    }

    void serialize(int item)
    {
        stream << item;
    }

    void serialize(unsigned int item)
    {
        stream << item;
    }

    void serialize(size_t item)
    {
        stream << item;
    }

    void serialize(float item)
    {
        stream << item;
    }

    void serialize(double item)
    {
        stream << item;
    }
};
} // namespace mini_json::_private

