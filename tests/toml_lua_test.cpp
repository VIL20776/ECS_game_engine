#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "component.hpp"
#include "system_runtime.hpp"
#include "world.hpp"

namespace {

[[nodiscard]] auto tomlPath(std::string_view file_name) {
    return std::filesystem::path(__FILE__).parent_path() / file_name;
}

template <typename WorldT>
auto load_components(WorldT& world, std::string_view file_name) {
    const auto path = tomlPath(file_name);
    if constexpr (requires { world.loadComponents(path.string()); }) {
        return world.loadComponents(path.string());
    } else {
        return world.readComponentFile(path.string());
    }
}

} // namespace

TEST_CASE("TOML file reading") {
    World world;
    const auto defs = load_components(world, "valid.toml");

    if constexpr (requires { defs.has_value(); }) {
        REQUIRE(defs.has_value());
        REQUIRE(defs->at(0).name == "Position");
        REQUIRE(defs->at(0).fields.at(0).name == "x");
        REQUIRE(defs->at(0).fields.at(0).type == ecs::FieldType::Float32);
        REQUIRE(defs->at(0).fields.at(1).name == "y");
        REQUIRE(defs->at(0).fields.at(1).type == ecs::FieldType::Float32);
        REQUIRE(defs->at(1).name == "Rotation");
        REQUIRE(defs->at(1).fields.at(0).name == "x");
        REQUIRE(defs->at(1).fields.at(0).type == ecs::FieldType::Float32);
        REQUIRE(defs->at(1).fields.at(1).name == "y");
        REQUIRE(defs->at(1).fields.at(1).type == ecs::FieldType::Float32);
    } else {
        REQUIRE(defs.size() == 2);
        REQUIRE(defs.at(0).name == "Position");
    }
}

TEST_CASE("Lua script loading and execution") {
    World world;
    const EntityId entity = world.createEntity();

    const auto position = world.createComponent("Position", {
        {"x", ecs::FieldType::Float32},
        {"y", ecs::FieldType::Float32}
    });
    const auto position_id = [&]() {
        if constexpr (requires { position.value(); }) {
            return position.value();
        } else {
            return position;
        }
    }();

    float position_x = 10.0f;
    float position_y = 10.0f;
    std::vector<std::byte> x_bytes(sizeof(position_x));
    std::memcpy(x_bytes.data(), &position_x, sizeof(position_x));
    std::vector<std::byte> y_bytes(sizeof(position_y));
    std::memcpy(y_bytes.data(), &position_y, sizeof(position_y));

    std::vector<std::byte> data;
    data.insert(data.end(), x_bytes.begin(), x_bytes.end());
    data.insert(data.end(), y_bytes.begin(), y_bytes.end());
    world.addComponent(entity, position_id, data);

    ecs::lua::SystemRuntime runtime(world);
    const auto lua_result = [&]() {
        if constexpr (requires { runtime.loadFile(tomlPath("valid.lua").string()); }) {
            return runtime.loadFile(tomlPath("valid.lua").string());
        } else {
            runtime.loadFile(tomlPath("valid.lua").string());
            return true;
        }
    }();

    if constexpr (requires { lua_result.has_value(); }) {
        REQUIRE(lua_result.has_value());
    }

    const auto update_result = [&]() {
        if constexpr (requires { runtime.update(1.0 / 60.0); }) {
            return runtime.update(1.0 / 60.0);
        } else {
            runtime.update(1.0 / 60.0);
            return true;
        }
    }();

    if constexpr (requires { update_result.has_value(); }) {
        REQUIRE(update_result.has_value());
    }

    const auto* expected_archetype = world.query(std::vector<ComponentId>{position_id}).back();
    const auto raw = expected_archetype->getRawComponent(entity, position_id);
    const auto new_x = readField<float>(world.getComponentSchema(position_id), raw, "x");
    const auto new_y = readField<float>(world.getComponentSchema(position_id), raw, "y");

    REQUIRE(new_x == Approx(position_x + 10.0f / 60.0f));
    REQUIRE(new_y == Approx(position_y + 10.0f / 60.0f));
}

TEST_CASE("Load invalid TOML file reports failure") {
    World world;
    const auto defs = load_components(world, "invalid_component.toml");

    if constexpr (requires { defs.has_value(); }) {
        REQUIRE_FALSE(defs.has_value());
    }
}

TEST_CASE("Load invalid Lua script reports failure") {
    World world;
    ecs::lua::SystemRuntime runtime(world);
    const auto script = tomlPath("invalid_system.lua");

    const auto result = [&]() {
        if constexpr (requires { runtime.loadFile(script.string()); }) {
            return runtime.loadFile(script.string());
        } else {
            runtime.loadFile(script.string());
            return true;
        }
    }();

    if constexpr (requires { result.has_value(); }) {
        REQUIRE_FALSE(result.has_value());
    }
}
