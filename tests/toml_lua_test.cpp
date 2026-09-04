#include <catch2/catch_test_macros.hpp>

#include "component.hpp"
#include "loader.hpp"

TEST_CASE( "TOML file reading" ) {
    Loader loader (".");

    if (const auto defs = loader.readComponentFile("/valid.toml"); defs.has_value()) {
        SUCCEED("Success loading file.");
        REQUIRE(defs->at(0).name == "Position");
        auto& position = defs->at(0);
            REQUIRE(position.fields.at(0).first == "x");
            REQUIRE(position.fields.at(0).second == ecs::FieldType::Float32);
            REQUIRE(position.fields.at(1).first == "y");
            REQUIRE(position.fields.at(1).second == ecs::FieldType::Float32);
        REQUIRE(defs->at(1).name == "Rotation");
        auto& rotation = defs->at(1);
            REQUIRE(rotation.fields.at(0).first == "x");
            REQUIRE(rotation.fields.at(0).second == ecs::FieldType::Float32);
            REQUIRE(rotation.fields.at(1).first == "y");
            REQUIRE(rotation.fields.at(1).second == ecs::FieldType::Float32);
    } else {
        FAIL("Failed loading file.");
    }
}
