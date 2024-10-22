#include "json.h"
#include "gtest/gtest.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace mini_json;
using namespace std::string_literals;



template<typename T>
struct Vector2
{
    Vector2()
        : Vector2(0, 0) {}
    explicit
    Vector2(T x)
        : Vector2(x, x) {}
    Vector2(T x, T y)
        : x(x), y(y) {}
    template<typename Ty>
    Vector2(Vector2<Ty> oth)
        : Vector2(oth.x, oth.y) {}

    T x, y;
};

using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned>;
using Vector2f = Vector2<float>;

template<typename T>
class mini_json::SerializeInfo<Vector2<T>>
{

public:
    static constexpr auto fields()
    {
        using std::make_tuple;
        return make_tuple
                (
                    prop("x")[&Vector2<T>::x],
                    prop("y")[&Vector2<T>::y]
                );
    }

};


namespace
{



struct Seed
{
    float radius = 0.0;

    constexpr static auto json_properties()
    {
        return std::make_tuple(mini_json::property(&Seed::radius, "radius"));
    }
};



struct Apple
{
    std::string color = "";
    std::optional<int> size;
    std::variant<Seed, float> seed;

    const std::variant<Seed, float>& get_seed() const { return seed; }
    void set_seed(std::variant<Seed, float>&& seed) { this->seed = seed; }

    constexpr static auto json_properties()
    {
        return std::make_tuple(mini_json::property(&Apple::color, "color"),
                               mini_json::prop("seed")[&Apple::get_seed](&Apple::set_seed),
                               mini_json::property(&Apple::size, "size"));
    }
};

}


template<>
struct mini_json::SerializeInfo<Seed>
{
    static constexpr auto fields()
    {
        return
        make_tuple(
            prop("radius")[&Seed::radius]
        );
    }
};


namespace
{

class TestJsonParser : public ::testing::Test
{
protected:
};

TEST_F(TestJsonParser, CanReadJsonIntoObject)
{
    const auto json
        = R"a(
            {
                "color":"red",
                "size": -25,
                "seed": {
                    "radius": -3.14
                }
            })a"s;

    auto result = mini_json::parse<Apple>(json.begin(), json.end());

    EXPECT_EQ(result.color, "red");
    EXPECT_EQ(result.size.value(), -25);
    EXPECT_FLOAT_EQ(std::get<Seed>(result.seed).radius, -3.14f);

    {
        const auto json
            = R"a(
                {
                    "color":"red",
                    "seed": -3.14
                })a"s;

        auto result = mini_json::parse<Apple>(json.begin(), json.end());

        EXPECT_EQ(result.color, "red");
        EXPECT_EQ(not result.size.has_value(), true);
        EXPECT_FLOAT_EQ(std::get<float>(result.seed), -3.14f);
    }
}

TEST_F(TestJsonParser, CanReadQuotes)
{
    const auto json = R"a({"color":"\"red\"", })a"s;

    auto result = mini_json::parse<Apple>(json.begin(), json.end());

    EXPECT_EQ(result.color, "\"red\"");
}

TEST_F(TestJsonParser, ThrowsParseErrorOnInvalidJson)
{
    auto json = R"a({asd "color": "red","size": -25})a"s;
    EXPECT_THROW(mini_json::parse<Apple>(json.begin(), json.end()), mini_json::ParseError);

    json = R"a({"color"asd: "red","size": -25})a"s;
    EXPECT_THROW(mini_json::parse<Apple>(json.begin(), json.end()), mini_json::ParseError);

    json = R"a({"color"asd: "red","size": -2asd5})a"s;
    EXPECT_THROW(mini_json::parse<Apple>(json.begin(), json.end()), mini_json::ParseError);
}

struct AppleTree
{
    std::string id = "";
    std::vector<Apple> apples = {};

    void set_apples(std::vector<Apple>&& apples) { this->apples = std::move(apples); }
    const std::vector<Apple>& get_apples() const { return apples; }

    constexpr static auto json_properties()
    {
        return std::make_tuple(mini_json::property(&AppleTree::set_apples, [](const AppleTree* cls) -> const std::vector<Apple>& { return cls->apples; }, "apples"),
                               mini_json::property([](AppleTree* cls, std::string&& id) { cls->id = std::move(id); }, &AppleTree::id, "id"));
    }
};


TEST_F(TestJsonParser, CanReadObjectWithVectorOfObjects)
{
    const auto json = R"a({
        "apples": [
              {"color":"red","size":0,"seed":{"radius":0}},
              {"color":"red","size":1,"seed":{"radius":1}},
              {"color":"red","size":2,"seed":{"radius":2}},
              {"color":"red","size":3,"seed":{"radius":3}},
              {"color":"red","size":4,"seed":{"radius":4}}
        ]
    })a"s;


    auto sresult = std::stringstream();
    mini_json::serialize(Vector2u{}, sresult);

    auto result = mini_json::parse<AppleTree>(json.begin(), json.end());
    mini_json::serialize(AppleTree{}, sresult);

    ASSERT_EQ(result.apples.size(), 5u);

    float i = 0.0;
    for (auto& apple : result.apples)
    {
        EXPECT_EQ(apple.color, "red");
        EXPECT_EQ(apple.size, i);
        EXPECT_FLOAT_EQ(std::get<Seed>(apple.seed).radius, i);
        ++i;
    }
}

TEST_F(TestJsonParser, CanParseStreams)
{
    const auto json = "{\"apples\": ["
                      "{\"color\":\"red\",\"size\":0,\"seed\":{\"radius\":0}},"
                      "{\"color\":\"red\",\"size\":1,\"seed\":{\"radius\":1}},"
                      "{\"color\":\"red\",\"size\":2,\"seed\":{\"radius\":2}},"
                      "{\"color\":\"red\",\"size\":3,\"seed\":{\"radius\":3}},"
                      "{\"color\":\"red\",\"size\":4,\"seed\":{\"radius\":4}}"
                      "]}"s;

    auto stream = std::stringstream();
    stream << json;

    auto result = mini_json::parse<AppleTree>(stream);

    ASSERT_EQ(result.apples.size(), 5u);

    float i = 0;
    for (auto& apple : result.apples)
    {
        EXPECT_EQ(apple.color, "red");
        EXPECT_EQ(apple.size, i);
        EXPECT_FLOAT_EQ(std::get<Seed>(apple.seed).radius, i);
        ++i;
    }
}

}



int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
