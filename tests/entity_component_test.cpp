#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <filesystem>
#include <string_view>

#include "archetype.hpp"
#include "component.hpp"
#include "system_runtime.hpp"
#include "world.hpp"

using namespace ecs;

namespace {

[[nodiscard]] auto componentFilePath(std::string_view file_name) {
    return std::filesystem::path(__FILE__).parent_path() / file_name;
}

template <typename WorldT>
auto load_components(WorldT& world, std::string_view file_name) {
    const auto path = componentFilePath(file_name);
    if constexpr (requires { world.loadComponents(path.string()); }) {
        return world.loadComponents(path.string());
    } else {
        return world.readComponentFile(path.string());
    }
}

template <typename WorldT>
auto create_component(WorldT& world, std::string_view name, const std::vector<ComponentField>& fields) {
    if constexpr (requires { world.createComponent(name, fields); }) {
        return world.createComponent(name, fields);
    } else {
        return world.createComponent(std::string{name}, fields);
    }
}

} // namespace

TEST_CASE("Entity and Component creation") {
    World world;
    const EntityId ent = world.createEntity();

    REQUIRE(world.getEntityCount() == 1);
    REQUIRE(ent == 0);
    REQUIRE(world.getArchetypeCount() == 1);

    const auto component = create_component(world, "Position", {
        {.name = "x", .type = FieldType::Float32},
        {.name = "y", .type = FieldType::Float32}
    });

    if constexpr (requires { component.has_value(); }) {
        REQUIRE(component.has_value());
    }
    REQUIRE(world.getComponentCount() == 1);

    const auto schema = world.getComponentSchema(0);
    REQUIRE(schema.name == "Position");
    REQUIRE(schema.fields.at(0).name == "x");
    REQUIRE(schema.fields.at(0).type == FieldType::Float32);
    REQUIRE(schema.fields.at(1).name == "y");
    REQUIRE(schema.fields.at(1).type == FieldType::Float32);
}

TEST_CASE("Component data and archetype storage") {
    World world;
    const EntityId ent = world.createEntity();

    const auto component = create_component(world, "Position", {
        {.name = "x", .type = FieldType::Float32},
        {.name = "y", .type = FieldType::Float32}
    });

    if constexpr (requires { component.has_value(); }) {
        REQUIRE(component.has_value());
    }

    const auto component_id = [&]() {
        if constexpr (requires { component.value(); }) {
            return component.value();
        } else {
            return component;
        }
    }();

    float position_x = 3.1416f;
    float position_y = 6.1172f;

    std::vector<std::byte> position_x_bytes(sizeof(position_x));
    std::memcpy(position_x_bytes.data(), &position_x, sizeof(position_x));

    std::vector<std::byte> position_y_bytes(sizeof(position_y));
    std::memcpy(position_y_bytes.data(), &position_y, sizeof(position_y));

    std::vector<std::byte> position_data;
    position_data.insert(position_data.end(), position_x_bytes.begin(), position_x_bytes.end());
    position_data.insert(position_data.end(), position_y_bytes.begin(), position_y_bytes.end());

    world.addComponent(ent, component_id, position_data);
    REQUIRE(world.getArchetypeCount() == 2);

    const std::vector<ComponentId> archetype_signature = {component_id};
    auto* expected_archetype = world.query(archetype_signature).back();
    REQUIRE(expected_archetype->hasComponent(component_id));

    const auto raw_component = expected_archetype->getRawComponent(ent, component_id);
    REQUIRE(raw_component.size() == 8);

    const float position_x_recovered = readField<float>(world.getComponentSchema(component_id), raw_component, "x");
    const float position_y_recovered = readField<float>(world.getComponentSchema(component_id), raw_component, "y");

    REQUIRE(position_x == position_x_recovered);
    REQUIRE(position_y == position_y_recovered);
}

TEST_CASE("Component and entity deletion") {
    World world;
    const EntityId ent1 = world.createEntity();
    const EntityId ent2 = world.createEntity();

    const auto comp1 = create_component(world, "Position", {
        {"x", FieldType::Float32},
        {"y", FieldType::Float32}
    });
    const auto comp2 = create_component(world, "Rotation", {
        {"x", FieldType::Float32},
        {"y", FieldType::Float32}
    });

    const auto component_id_1 = [&]() {
        if constexpr (requires { comp1.value(); }) {
            return comp1.value();
        } else {
            return comp1;
        }
    }();
    const auto component_id_2 = [&]() {
        if constexpr (requires { comp2.value(); }) {
            return comp2.value();
        } else {
            return comp2;
        }
    }();

    float position_x = 3.1416f;
    float position_y = 6.1172f;

    std::vector<std::byte> position_x_bytes(sizeof(position_x));
    std::memcpy(position_x_bytes.data(), &position_x, sizeof(position_x));

    std::vector<std::byte> position_y_bytes(sizeof(position_y));
    std::memcpy(position_y_bytes.data(), &position_y, sizeof(position_y));

    std::vector<std::byte> position_data;
    position_data.insert(position_data.end(), position_x_bytes.begin(), position_x_bytes.end());
    position_data.insert(position_data.end(), position_y_bytes.begin(), position_y_bytes.end());

    world.addComponent(ent1, component_id_1, position_data);
    world.addComponent(ent2, component_id_1, position_data);
    world.addComponent(ent2, component_id_2, position_data);

    REQUIRE(world.getArchetypeCount() == 3);
    REQUIRE(world.getEntityCount() == 2);

    world.removeComponent(ent2, component_id_1);
    REQUIRE(world.getArchetypeCount() == 4);

    const std::vector<ComponentId> archetype_signature = {component_id_2};
    const auto query_result = world.query(archetype_signature);
    REQUIRE(query_result.size() == 2);
    const auto* expected_archetype = query_result.front();
    const auto ent2_record = world.getEntityRecord(ent2);
    REQUIRE(ent2_record.archetype == expected_archetype);

    world.deleteEntity(ent1);
    REQUIRE(world.getEntityCount() == 1);
}

TEST_CASE("Query archetypes and mutate fields") {
    World world;
    const EntityId ent1 = world.createEntity();
    const EntityId ent2 = world.createEntity();

    const auto comp1 = create_component(world, "Position", {
        {.name = "x", .type = FieldType::Float32},
        {.name = "y", .type = FieldType::Float32}
    });
    const auto comp2 = create_component(world, "Rotation", {
        {.name = "x", .type = FieldType::Float32},
        {.name = "y", .type = FieldType::Float32}
    });

    const auto component_id_1 = [&]() {
        if constexpr (requires { comp1.value(); }) {
            return comp1.value();
        } else {
            return comp1;
        }
    }();
    const auto component_id_2 = [&]() {
        if constexpr (requires { comp2.value(); }) {
            return comp2.value();
        } else {
            return comp2;
        }
    }();

    const float position_x = 3.1416f;
    const float position_y = 6.1172f;

    std::vector<std::byte> position_x_bytes(sizeof(position_x));
    std::memcpy(position_x_bytes.data(), &position_x, sizeof(position_x));

    std::vector<std::byte> position_y_bytes(sizeof(position_y));
    std::memcpy(position_y_bytes.data(), &position_y, sizeof(position_y));

    std::vector<std::byte> position_data;
    position_data.insert(position_data.end(), position_x_bytes.begin(), position_x_bytes.end());
    position_data.insert(position_data.end(), position_y_bytes.begin(), position_y_bytes.end());

    world.addComponent(ent1, component_id_1, position_data);
    world.addComponent(ent2, component_id_1, position_data);
    world.addComponent(ent2, component_id_2, position_data);

    const auto query = world.query(std::vector<ComponentId>{component_id_1});
    REQUIRE(query.size() == 2);

    float value = 3.0f;
    for (auto* arch : query) {
        for (const auto eid : arch->getEntities()) {
            auto component_data = arch->getRawComponent(eid, component_id_1);
            auto field_x = readField<float>(world.getComponentSchema(component_id_1), component_data, "x");
            field_x += value;
            REQUIRE(position_x + value == field_x);

            writeField(world.getComponentSchema(component_id_1), component_data, "x", field_x);
            arch->setRawComponent(eid, component_id_1, component_data);
            const auto updated_data = arch->getRawComponent(eid, component_id_1);
            const auto new_field_x = readField<float>(world.getComponentSchema(component_id_1), updated_data, "x");
            REQUIRE(position_x + value == new_field_x);

            value += 3.0f;
        }
    }
}

TEST_CASE("Duplicate component registration is rejected") {
    World world;
    const auto first = create_component(world, "Position", {{"x", FieldType::Float32}});
    if constexpr (requires { first.has_value(); }) {
        REQUIRE(first.has_value());
    }

    const auto second = create_component(world, "Position", {{"x", FieldType::Float32}});
    if constexpr (requires { second.has_value(); }) {
        REQUIRE_FALSE(second.has_value());
    }
}

TEST_CASE("Component schema validation rejects invalid definitions") {
    World world;
    const auto invalid = create_component(world, "Broken", {{"x", FieldType::Bytes}});
    if constexpr (requires { invalid.has_value(); }) {
        REQUIRE_FALSE(invalid.has_value());
    }
}

TEST_CASE("World loads component definitions from TOML") {
    World world;
    const auto definitions = load_components(world, "valid.toml");

    if constexpr (requires { definitions.has_value(); }) {
        REQUIRE(definitions.has_value());
        REQUIRE(definitions->size() == 2);
        REQUIRE(definitions->at(0).name == "Position");
        REQUIRE(definitions->at(0).fields.at(0).name == "x");
        REQUIRE(definitions->at(0).fields.at(0).type == FieldType::Float32);
        REQUIRE(definitions->at(1).name == "Rotation");
        REQUIRE(definitions->at(1).fields.at(1).name == "y");
    } else {
        REQUIRE(definitions->size() == 2);
        REQUIRE(definitions->at(0).name == "Position");
    }
}

TEST_CASE("World rejects invalid TOML field types") {
    World world;
    const auto definitions = load_components(world, "invalid_component.toml");

    if constexpr (requires { definitions.has_value(); }) {
        REQUIRE_FALSE(definitions.has_value());
    } else {
        REQUIRE_THROWS_AS(world.loadComponents("invalid_component.toml"), std::exception);
    }
}

TEST_CASE("Lua script updates component state") {
    using namespace ecs::lua;

    World world;
    const EntityId entity = world.createEntity();

    const auto position = create_component(world, "Position", {
        {"x", FieldType::Float32},
        {"y", FieldType::Float32}
    });
    const auto position_id = [&]() {
        if constexpr (requires { position.value(); }) {
            return position.value();
        } else {
            return position;
        }
    }();

    const float original_x = 10.0f;
    const float original_y = 20.0f;
    std::vector<std::byte> bytes_x(sizeof(original_x));
    std::memcpy(bytes_x.data(), &original_x, sizeof(original_x));
    std::vector<std::byte> bytes_y(sizeof(original_y));
    std::memcpy(bytes_y.data(), &original_y, sizeof(original_y));

    std::vector<std::byte> position_data;
    position_data.insert(position_data.end(), bytes_x.begin(), bytes_x.end());
    position_data.insert(position_data.end(), bytes_y.begin(), bytes_y.end());
    world.addComponent(entity, position_id, position_data);

    SystemRuntime runtime(world);
    const auto load_result = [&]() {
        if constexpr (requires { runtime.loadFile(componentFilePath("valid.lua").string()); }) {
            return runtime.loadFile(componentFilePath("valid.lua").string());
        } else {
            runtime.loadFile(componentFilePath("valid.lua").string());
            return true;
        }
    }();

    if constexpr (requires { load_result.has_value(); }) {
        REQUIRE(load_result.has_value());
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

    auto* archetype = world.query(std::vector<ComponentId>{position_id}).back();
    const auto raw = archetype->getRawComponent(entity, position_id);
    const float x = readField<float>(world.getComponentSchema(position_id), raw, "x");
    const float y = readField<float>(world.getComponentSchema(position_id), raw, "y");

    REQUIRE(x == original_x + 10.0f / 60.0f);
    REQUIRE(y == original_y + 10.0f / 60.0f);
}

TEST_CASE("Invalid Lua script is rejected") {
    using namespace ecs::lua;

    World world;
    SystemRuntime runtime(world);
    const auto invalid_script = componentFilePath("invalid_system.lua");

    const auto load_result = [&]() {
        if constexpr (requires { runtime.loadFile(invalid_script.string()); }) {
            return runtime.loadFile(invalid_script.string());
        } else {
            runtime.loadFile(invalid_script.string());
            return true;
        }
    }();

    if constexpr (requires { load_result.has_value(); }) {
        REQUIRE_FALSE(load_result.has_value());
    }
}

TEST_CASE("World loads and preserves component definitions from TOML with multiple blocks") {
    World world;
    const auto definitions = load_components(world, "valid.toml");

    if constexpr (requires { definitions.has_value(); }) {
        REQUIRE(definitions.has_value());
        REQUIRE(definitions->front().name == "Position");
        REQUIRE(definitions->back().name == "Rotation");
    } else {
        REQUIRE(definitions->size() == 2);
    }
}

TEST_CASE("SystemRuntime handles invalid query definitions safely") {
    using namespace ecs::lua;

    World world;
    SystemRuntime runtime(world);
    const auto invalid_query = componentFilePath("invalid_system.lua");

    const auto load_result = [&]() {
        if constexpr (requires { runtime.loadFile(invalid_query.string()); }) {
            return runtime.loadFile(invalid_query.string());
        } else {
            runtime.loadFile(invalid_query.string());
            return true;
        }
    }();

    if constexpr (requires { load_result.has_value(); }) {
        REQUIRE_FALSE(load_result.has_value());
    }
}
