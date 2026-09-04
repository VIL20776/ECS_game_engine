#pragma once

#include "archetype.hpp"
#include "component.hpp"
#include "entity.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ecs {


class World {
public:
    World();

    // Registers a reflected component schema. The caller must provide the name and
    // fields of the component.
    ComponentId createComponent(std::string_view name, const std::vector<ComponentField>& fields);

    // Removes a component definition. To keep entity/archetype invariants
    // explicit, schemas currently in use cannot be removed.
    void deleteComponent(ComponentId component_id);

    // Creates an entity directly in the empty archetype
    EntityId createEntity();

    void deleteEntity(EntityId entity_id);

    // Adds a component to an existing entity by moving it to the archetype
    // represented by its current signature plus component_id. Existing
    // component bytes are preserved and the new component is zero-initialized.
    // void addComponent(EntityId entity_id, ComponentId component_id);

    void addComponent(
            EntityId entity_id, 
            ComponentId component_id, 
            const std::vector<std::byte>& data);

    // Removes a component from an existing entity by moving it to the archetype
    // represented by its current signature minus component_id. Remaining
    // component bytes are preserved.
    void removeComponent(EntityId entity_id, ComponentId component_id);

    // Returns every archetype whose signature is a superset of component_ids.
    std::vector<Archetype*> query(std::span<const ComponentId> component_ids);

    const ComponentSchema& getComponentSchema(ComponentId component_id);

    const EntityRecord& getEntityRecord(EntityId entity_id);

    std::size_t getEntityCount();
    std::size_t getComponentCount();
    std::size_t getArchetypeCount();

private:

    // void moveEntityToSignature(EntityId entity_id, ComponentId component_id, bool add_component);

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
    std::unordered_map<Signature, Archetype::Edge, Archetype::Hash> archetype_map;
};

} // namespace ecs
