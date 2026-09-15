#include "world.hpp"

#include "archetype.hpp"
#include "component.hpp"
#include "entity.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ecs {

World::World() {}

ComponentId World::createComponent(std::string_view name, const std::vector<ComponentField>& fields) 
{
    ComponentSchema schema;

    if (schema.id == InvalidComponentId) {
        schema.id = next_component_id++;
    } else {
        next_component_id = std::max(next_component_id, schema.id + 1);
    }

    schema.name = name;
    schema.fields = fields;

    std::size_t offset = 0;
    for (auto& field: schema.fields) {
        field.size = fieldTypeSize(field.type);
        field.offset = offset;
        offset += field.size;
    }

    schema.size = offset; 

    if (!schema.validate()) {
        throw std::invalid_argument("invalid component schema");
    }
    if (component_schemas.contains(schema.id)) {
        throw std::invalid_argument("component ID is already registered");
    }

    const ComponentId component_id = schema.id;
    auto [item, inserted] = component_schemas.emplace(component_id, std::move(schema));
    component_schema_map.emplace(name, item->second);
    return component_id;
}

void World::deleteComponent(ComponentId component_id) {
    if (!component_schemas.contains(component_id)) {
        return;
    }

    for (const auto& [archetype_id, archetype_ptr] : archetypes) {
        (void)archetype_id;
        if (archetype_ptr->hasComponent(component_id) && !archetype_ptr->getEntities().empty()) {
            throw std::logic_error("cannot destroy a component used by live entities");
        }
    }

    component_schemas.erase(component_id);
    removeUnusedArchetypes();
}

EntityId World::createEntity() { 
    Archetype& archetype = getOrCreateArchetype({});
    const EntityId entity_id = next_entity_id++;
    const std::size_t row = archetype.appendEntity(entity_id);
    entity_records.emplace(entity_id, EntityRecord{&archetype, row});
    return entity_id;
}

void World::deleteEntity(EntityId entity_id) 
{
    const auto record_it = entity_records.find(entity_id);
    if (record_it == entity_records.end()) 
        return;
    
    Archetype& archetype = *record_it->second.archetype;
    const std::size_t removed_row = record_it->second.row;
    const std::size_t last_row = archetype.getEntities().size() - 1;
    const EntityId moved_entity_id = archetype.getEntities()[last_row];

    archetype.removeEntity(entity_id);
    entity_records.erase(record_it);

    if (removed_row != last_row) 
        entity_records.at(moved_entity_id).row = removed_row; 
}

void World::addComponent(EntityId entity_id, ComponentId component_id, const std::vector<std::byte>& data) {
    migrateEntity(entity_id, component_id);
    EntityRecord record = entity_records.at(entity_id);
    record.archetype->setRawComponent(entity_id, component_id, data);
}

void World::removeComponent(EntityId entity_id, ComponentId component_id) {
    migrateEntity(entity_id, component_id);
}

// Returns every archetype whose signature is a superset of component_ids.
std::vector<Archetype*> 
World::query(std::span<const ComponentId> component_ids) 
{
    std::vector<ComponentId> required{component_ids.begin(), component_ids.end()};
    normalizeSignature(required);
    validateSignature(required);

    std::vector<Archetype*> result;
    for (auto& [archetype_id, archetype_ptr] : archetypes) {
        (void)archetype_id;
        const auto signature = archetype_ptr->getComponentIds();
        if (std::includes(signature.begin(), signature.end(), required.begin(), required.end())) {
            result.push_back(archetype_ptr.get());
        }
    }
    return result;
}

ComponentSchema&
World::getComponentSchema(ComponentId component_id) { return component_schemas.at(component_id); }

ComponentSchema&
World::getComponentSchema(const std::string& component_name) { return component_schema_map.at(component_name); }

const EntityRecord&
World::getEntityRecord(EntityId entity_id) { return entity_records.at(entity_id); }

std::size_t
World::getEntityCount() { return entity_records.size(); }

std::size_t
World::getComponentCount() { return component_schemas.size(); }

std::size_t
World::getArchetypeCount() { return archetypes.size(); }

void World::migrateEntity(EntityId entity_id, ComponentId component_id) 
{
    const auto record_it = entity_records.find(entity_id);
    if (record_it == entity_records.end()) {
        throw std::out_of_range("unknown entity");
    }
    if (!component_schemas.contains(component_id)) {
        throw std::invalid_argument("unknown component");
    }

    Archetype* source = record_it->second.archetype;
    Archetype* target = nullptr;

    std::vector<ComponentId> target_signature {
        source->getComponentIds().begin(),
        source->getComponentIds().end()
    };

    if (source->component_edge.contains(component_id)) {
        auto& edge = source->component_edge.at(component_id);
        target = (edge.add != nullptr) ? edge.add: edge.remove;
    } else {
        const auto component_it = std::lower_bound(
                target_signature.begin(), target_signature.end(), component_id);
        const bool has_component =
            component_it != target_signature.end() && *component_it == component_id;

        if (has_component) {
            target_signature.erase(component_it);
        } else {
            target_signature.insert(component_it, component_id);
        }

        target = &getOrCreateArchetype(target_signature);

        if (source->hasComponent(component_id)) {
            source->component_edge.insert_or_assign(component_id, Archetype::Edge{.remove = target});
            target->component_edge.insert_or_assign(component_id, Archetype::Edge{.add = source});
        } else {
            source->component_edge.insert_or_assign(component_id, Archetype::Edge{.add = target});
            target->component_edge.insert_or_assign(component_id, Archetype::Edge{.remove = source});
        }
    }

    const std::size_t target_row = target->appendEntity(entity_id);

    // Copy every component shared by both archetypes. Components newly
    // introduced in target keep appendEntity()'s zero initialization.
    for (ComponentId shared_component_id : target_signature) {
        if (!source->hasComponent(shared_component_id)) {
            continue;
        }
        target->setRawComponent(
            entity_id,
            shared_component_id,
            source->getRawComponent(entity_id, shared_component_id));
    }

    const std::size_t source_row = record_it->second.row;
    const std::size_t source_last_row = source->getEntities().size() - 1;
    const EntityId moved_entity_id = source->getEntities()[source_last_row];

    source->removeEntity(entity_id);
    record_it->second = EntityRecord{target, target_row};

    if (source_row != source_last_row) {
        entity_records.at(moved_entity_id).row = source_row;
    }
}

void World::normalizeSignature(std::vector<ComponentId>& signature) 
{
    std::sort(signature.begin(), signature.end());
    signature.erase(std::unique(signature.begin(), signature.end()), signature.end());
}

void World::validateSignature(std::span<const ComponentId> signature) 
{
    for (ComponentId component_id : signature) {
        if (!component_schemas.contains(component_id)) {
            throw std::invalid_argument("entity/archetype references an unknown component");
        }
    }
}

Archetype* World::findArchetype(const Signature& signature) 
{
    const auto& archetype_it = archetypes.find(signature);
    if (archetype_it != archetypes.end())
        return archetype_it->second.get();

    return nullptr;
}

Archetype& World::getOrCreateArchetype(const std::vector<ComponentId>& signature)
{
    if (Archetype* existing = findArchetype(signature)) {
        return *existing;
    }

    // const ArchetypeId archetype_id = next_archetype_id++;
    auto archetype_ptr = std::unique_ptr<Archetype>(
        new Archetype(signature, component_schemas));
    Archetype& archetype = *archetype_ptr;
    archetypes.emplace(signature, std::move(archetype_ptr));
    return archetype;
}

void World::removeUnusedArchetypes()
{
    for (auto it = archetypes.begin(); it != archetypes.end();) {
        if (it->second->getEntities().empty()) {
            it = archetypes.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ecs
