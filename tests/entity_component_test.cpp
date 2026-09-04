#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "archetype.hpp"
#include "component.hpp"
#include "world.hpp"

using namespace ecs;

TEST_CASE( "Entity and Component creation" ) {
    World world = World();
    EntityId ent = world.createEntity();

    REQUIRE(world.getEntityCount() == 1);
    REQUIRE(ent == 0);
    REQUIRE(world.getArchetypeCount() == 1);

    ComponentId comp = world.createComponent("Position", {
            {.name = "x", .type = FieldType::Float32},
            {.name = "y", .type = FieldType::Float32}
            });

    REQUIRE(world.getComponentCount() == 1);
    REQUIRE(comp == 0);

    auto schema = world.getComponentSchema(comp);

    REQUIRE(schema.name == "Position");
    REQUIRE(schema.fields.at(0).name == "x");
    REQUIRE(schema.fields.at(0).type == FieldType::Float32);
    REQUIRE(schema.fields.at(1).name == "y");
    REQUIRE(schema.fields.at(1).type == FieldType::Float32);
}

TEST_CASE("Component data and Archetype storage") {
    World world = World();
    EntityId ent = world.createEntity();

    ComponentId comp = world.createComponent("Position", {
            {.name = "x", .type = FieldType::Float32},
            {.name = "y", .type = FieldType::Float32}
            });

    float position_x = 3.1416;
    float position_y = 6.1172;

    std::vector<std::byte> position_x_bytes(sizeof(position_x));
    std::memcpy(position_x_bytes.data(), &position_x, sizeof(position_x));

    std::vector<std::byte> position_y_bytes(sizeof(position_y));
    std::memcpy(position_y_bytes.data(), &position_y, sizeof(position_y));

    std::vector<std::byte> position_data {};
    position_data.insert(position_data.end(), position_x_bytes.begin(), position_x_bytes.end());
    position_data.insert(position_data.end(), position_y_bytes.begin(), position_y_bytes.end());

    world.addComponent(ent, comp, position_data);
    REQUIRE(world.getArchetypeCount() == 2);

    std::vector<ComponentId> archetype_signature = {comp};
    Archetype* expected_archetype = world.query(archetype_signature).back();
    REQUIRE(expected_archetype->hasComponent(comp));

    auto raw_component = expected_archetype->getRawComponent(ent, comp);
    REQUIRE(raw_component.size() == 8);
    
    float position_x_recovered, position_y_recovered;
    position_x_recovered = readField<float>(world.getComponentSchema(comp), raw_component, "x");
    position_y_recovered = readField<float>(world.getComponentSchema(comp), raw_component, "y");

    REQUIRE(position_x == position_x_recovered);
    REQUIRE(position_y == position_y_recovered);
}

TEST_CASE("Component and entity deletion") {
    World world = World();
    EntityId ent1 = world.createEntity();
    EntityId ent2 = world.createEntity();

    ComponentId comp1 = world.createComponent("Position", {
            {"x", FieldType::Float32}, 
            {"y", FieldType::Float32}
            });
    ComponentId comp2 = world.createComponent("Rotation", {
            {"x", FieldType::Float32}, 
            {"y", FieldType::Float32}
            });

    float position_x = 3.1416;
    float position_y = 6.1172;

    std::vector<std::byte> position_x_bytes(sizeof(position_x));
    std::memcpy(position_x_bytes.data(), &position_x, sizeof(position_x));

    std::vector<std::byte> position_y_bytes(sizeof(position_y));
    std::memcpy(position_y_bytes.data(), &position_y, sizeof(position_y));

    std::vector<std::byte> position_data {};
    position_data.insert(position_data.end(), position_x_bytes.begin(), position_x_bytes.end());
    position_data.insert(position_data.end(), position_y_bytes.begin(), position_y_bytes.end());

    world.addComponent(ent1, comp1, position_data);

    world.addComponent(ent2, comp1, position_data);
    world.addComponent(ent2, comp2, position_data);

    REQUIRE(world.getArchetypeCount() == 3);
    REQUIRE(world.getEntityCount() == 2);

    world.removeComponent(ent2, comp1);
    REQUIRE(world.getArchetypeCount() == 4);

    std::vector<ComponentId> archetype_signature = {comp2};
    auto query_result = world.query(archetype_signature);
    REQUIRE(query_result.size() == 2);
    auto expected_archetype = query_result.front();
    auto ent2_record = world.getEntityRecord(ent2);
    REQUIRE(ent2_record.archetype == expected_archetype);

    world.deleteEntity(ent1);
    REQUIRE(world.getEntityCount() == 1);
}

TEST_CASE("Query archetypes") {
    World world = World();
    EntityId ent1 = world.createEntity();
    EntityId ent2 = world.createEntity();

    ComponentId comp1 = world.createComponent("Position", {
            {.name = "x", .type = FieldType::Float32}, 
            {.name = "y", .type = FieldType::Float32}
            });
    ComponentId comp2 = world.createComponent("Rotation", {
            {.name = "x", .type = FieldType::Float32},
            {.name = "y", .type = FieldType::Float32}
            });

    float position_x = 3.1416;
    float position_y = 6.1172;

    std::vector<std::byte> position_x_bytes(sizeof(position_x));
    std::memcpy(position_x_bytes.data(), &position_x, sizeof(position_x));

    std::vector<std::byte> position_y_bytes(sizeof(position_y));
    std::memcpy(position_y_bytes.data(), &position_y, sizeof(position_y));

    std::vector<std::byte> position_data {};
    position_data.insert(position_data.end(), position_x_bytes.begin(), position_x_bytes.end());
    position_data.insert(position_data.end(), position_y_bytes.begin(), position_y_bytes.end());

    world.addComponent(ent1, comp1, position_data);

    world.addComponent(ent2, comp1, position_data);
    world.addComponent(ent2, comp2, position_data);

    auto query = world.query(std::vector<ComponentId>{comp1});
    REQUIRE(query.size() == 2);

    for (auto arch: query) {
        float value = 3;
        for (auto eid: arch->getEntities()) {
            auto component_data = arch->getRawComponent(eid, comp1);
            auto field_x = readField<float>(world.getComponentSchema(comp1), component_data, "x");
            field_x += value;
            REQUIRE(position_x + value == field_x);

            writeField(world.getComponentSchema(comp1), component_data, "x", field_x);
            arch->setRawComponent(eid, comp1, component_data);
            auto new_component_data = arch->getRawComponent(eid, comp1);
            auto new_field_x = readField<float>(world.getComponentSchema(comp1), new_component_data, "x");
            REQUIRE(position_x + value == new_field_x);

            value += 3;
        }
    }
}
