#include <catch2/catch_test_macros.hpp>

#include "component.hpp"
#include "loader.hpp"
#include "system_runtime.hpp"

TEST_CASE( "TOML file reading" ) {
    Loader loader (".");

    if (const auto defs = loader.readComponentFile("/valid.toml"); defs.has_value()) {
        SUCCEED("Success loading file.");
        REQUIRE(defs->at(0).name == "Position");
        auto& position = defs->at(0);
            REQUIRE(position.fields.at(0).name == "x");
            REQUIRE(position.fields.at(0).type == ecs::FieldType::Float32);
            REQUIRE(position.fields.at(1).name == "y");
            REQUIRE(position.fields.at(1).type == ecs::FieldType::Float32);
        REQUIRE(defs->at(1).name == "Rotation");
        auto& rotation = defs->at(1);
            REQUIRE(rotation.fields.at(0).name == "x");
            REQUIRE(rotation.fields.at(0).type == ecs::FieldType::Float32);
            REQUIRE(rotation.fields.at(1).name == "y");
            REQUIRE(rotation.fields.at(1).type == ecs::FieldType::Float32);
    } else {
        FAIL("Failed loading file.");
    }
}

TEST_CASE( "Lua script loading file.") {
    ecs::World world;

    ecs::EntityId ent = world.createEntity();

    Loader loader (".");

    float position_x = 10;
    float position_y = 10;

    std::vector<std::byte> position_x_bytes(sizeof(position_x));
    std::memcpy(position_x_bytes.data(), &position_x, sizeof(position_x));

    std::vector<std::byte> position_y_bytes(sizeof(position_y));
    std::memcpy(position_y_bytes.data(), &position_y, sizeof(position_y));

    std::vector<std::byte> position_data {};
    position_data.insert(position_data.end(), position_x_bytes.begin(), position_x_bytes.end());
    position_data.insert(position_data.end(), position_y_bytes.begin(), position_y_bytes.end());

    std::vector<ecs::ComponentId> archetype_signature;
    if (const auto defs = loader.readComponentFile("/valid.toml"); defs.has_value()) {
        for (auto& def: defs.value()) {
            ecs::ComponentId cid = world.createComponent(def.name, def.fields);        
            world.addComponent(ent, cid, position_data);
            archetype_signature.push_back(cid);
        }
    }

    ecs::lua::SystemRuntime runtime(world);
    runtime.loadFile("./valid.lua");

    constexpr double delta_time = 1.0 / 60.0;
    runtime.update(delta_time);

    ecs::Archetype* expected_archetype = world.query(archetype_signature).back();
    auto raw_component = expected_archetype->getRawComponent(ent, archetype_signature.front());
    
    float position_x_recovered, position_y_recovered;
    position_x_recovered = readField<float>(world.getComponentSchema(archetype_signature.front()), raw_component, "x");
    position_y_recovered = readField<float>(world.getComponentSchema(archetype_signature.front()), raw_component, "y");

    REQUIRE(position_x_recovered == position_x + (float)(delta_time * 10));
    REQUIRE(position_y_recovered == position_y + (float)(delta_time * 10));
}
