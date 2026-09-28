#pragma once

#include "archetype.hpp"
#include "component.hpp"
#include "entity.hpp"

#include <cstddef>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ecs {

enum class ComponentLoadError {
    ParseError,
    FieldTypeError,
    InvalidDefinition,
    DuplicateComponent
};

enum class ComponentCreateError {
    InvalidSchema,
    DuplicateComponent,
    UnknownComponent
};

struct ComponentDefinition {
    std::string name;
    std::vector<ComponentField> fields;
};

class World {
public:
    World() = default;

    [[nodiscard]] std::expected<ComponentId, ComponentCreateError> createComponent(
        std::string_view name,
        const std::vector<ComponentField>& fields);

    void deleteComponent(ComponentId component_id);

    [[nodiscard]] EntityId createEntity();

    void deleteEntity(EntityId entity_id);

    void addComponent(
        EntityId entity_id,
        ComponentId component_id,
        const std::vector<std::byte>& data);

    void removeComponent(EntityId entity_id, ComponentId component_id);

    [[nodiscard]] std::vector<Archetype*> query(std::span<const ComponentId> component_ids);

    [[nodiscard]] std::expected<std::vector<ComponentDefinition>, ComponentLoadError> loadComponents(
        std::string_view file_path);

    [[nodiscard]] ComponentSchema& getComponentSchema(ComponentId component_id);
    [[nodiscard]] ComponentSchema& getComponentSchema(const std::string& component_name);

    [[nodiscard]] const EntityRecord& getEntityRecord(EntityId entity_id);

    // Returns all entity IDs currently in the world
    [[nodiscard]] std::vector<EntityId> getAllEntityIds() const;

    [[nodiscard]] std::size_t getEntityCount() const;
    [[nodiscard]] std::size_t getComponentCount() const;
    [[nodiscard]] std::size_t getArchetypeCount() const;

private:
    void migrateEntity(EntityId entity_id, ComponentId component_id);

    static void normalizeSignature(Signature& signature);

    void validateSignature(std::span<const ComponentId> signature);

    Archetype* findArchetype(const Signature& signature);

    Archetype& getOrCreateArchetype(const Signature& signature);

    void removeUnusedArchetypes();

    EntityId next_entity_id{0};
    ComponentId next_component_id{0};
    ArchetypeId next_archetype_id{0};

    std::unordered_map<ComponentId, ComponentSchema> component_schemas;
    std::unordered_map<EntityId, EntityRecord> entity_records;
    std::unordered_map<Signature, std::unique_ptr<Archetype>, Archetype::Hash> archetypes;

    std::unordered_map<std::string, ComponentSchema&> component_schema_map;
};

} // namespace ecs
