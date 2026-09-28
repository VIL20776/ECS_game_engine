#include "world.hpp"

#include "archetype.hpp"
#include "component.hpp"
#include "entity.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <print>
#include <span>
#include <stdexcept>
#include <string_view>
#include <toml++/toml.hpp>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ecs {

namespace {

constexpr std::string_view kTypeNameFloat = "f32";
constexpr std::string_view kTypeNameInt = "i32";

const std::unordered_map<std::string, FieldType> type_map = {
    {std::string{kTypeNameFloat}, FieldType::Float32},
    {std::string{kTypeNameInt}, FieldType::Int32},
};

} // namespace

std::expected<ComponentId, ComponentCreateError> World::createComponent(
    std::string_view name,
    const std::vector<ComponentField>& fields)
{
    ComponentSchema schema;
    schema.id = next_component_id++;
    schema.name = std::string{name};
    schema.fields = fields;

    std::size_t offset = 0;
    for (auto& field : schema.fields) {
        field.size = fieldTypeSize(field.type);
        field.offset = offset;
        offset += field.size;
    }
    schema.size = offset;

    if (!schema.validate()) {
        return std::unexpected(ComponentCreateError::InvalidSchema);
    }
    if (component_schemas.contains(schema.id)) {
        return std::unexpected(ComponentCreateError::DuplicateComponent);
    }

    const ComponentId component_id = schema.id;
    auto [item, inserted] = component_schemas.emplace(component_id, std::move(schema));
    if (!inserted) {
        return std::unexpected(ComponentCreateError::DuplicateComponent);
    }
    component_schema_map.emplace(std::string{name}, item->second);
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

void World::deleteEntity(EntityId entity_id) {
    const auto record_it = entity_records.find(entity_id);
    if (record_it == entity_records.end()) {
        return;
    }

    Archetype& archetype = *record_it->second.archetype;
    const std::size_t removed_row = record_it->second.row;
    const std::size_t last_row = archetype.getEntities().size() - 1;
    const EntityId moved_entity_id = archetype.getEntities()[last_row];

    archetype.removeEntity(entity_id);
    entity_records.erase(record_it);

    if (removed_row != last_row) {
        entity_records.at(moved_entity_id).row = removed_row;
    }
}

void World::addComponent(EntityId entity_id, ComponentId component_id, const std::vector<std::byte>& data) {
    migrateEntity(entity_id, component_id);
    const auto record = entity_records.at(entity_id);
    record.archetype->setRawComponent(entity_id, component_id, data);
}

void World::removeComponent(EntityId entity_id, ComponentId component_id) {
    migrateEntity(entity_id, component_id);
}

std::vector<Archetype*> World::query(std::span<const ComponentId> component_ids) {
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

std::expected<std::vector<ComponentDefinition>, ComponentLoadError> World::loadComponents(
    std::string_view file_path)
{
    std::vector<ComponentDefinition> definitions;
    std::string component_file_path{file_path};
    toml::table tbl;

    try {
        tbl = toml::parse_file(component_file_path);
    } catch (const toml::parse_error&) {
        return std::unexpected(ComponentLoadError::ParseError);
    }

    for (auto& item : tbl) {
        ComponentDefinition definition;
        definition.name = item.first.str();

        auto* table = item.second.as_table();
        if (table == nullptr) {
            return std::unexpected(ComponentLoadError::InvalidDefinition);
        }

        for (auto& field : *table) {
            const std::string field_name{field.first.str()};
            const auto field_value = field.second.value<std::string>();
            if (!field_value) {
                return std::unexpected(ComponentLoadError::InvalidDefinition);
            }
            const std::string field_type = *field_value;
            const auto it = type_map.find(field_type);
            if (it == type_map.end()) {
                return std::unexpected(ComponentLoadError::FieldTypeError);
            }
            definition.fields.emplace_back(field_name, it->second);
        }

        definitions.push_back(definition);
    }

    return definitions;
}

ComponentSchema& World::getComponentSchema(ComponentId component_id) { return component_schemas.at(component_id); }

ComponentSchema& World::getComponentSchema(const std::string& component_name) { return component_schema_map.at(component_name); }

const EntityRecord& World::getEntityRecord(EntityId entity_id) { return entity_records.at(entity_id); }

std::vector<EntityId> World::getAllEntityIds() const {
    std::vector<EntityId> entity_ids;
    entity_ids.reserve(entity_records.size());
    for (const auto& [entity_id, record] : entity_records) {
        (void)record;
        entity_ids.push_back(entity_id);
    }
    std::sort(entity_ids.begin(), entity_ids.end());
    return entity_ids;
}

std::size_t World::getEntityCount() const { return entity_records.size(); }

std::size_t World::getComponentCount() const { return component_schemas.size(); }

std::size_t World::getArchetypeCount() const { return archetypes.size(); }

void World::migrateEntity(EntityId entity_id, ComponentId component_id) {
    const auto record_it = entity_records.find(entity_id);
    if (record_it == entity_records.end()) {
        throw std::out_of_range("unknown entity");
    }
    if (!component_schemas.contains(component_id)) {
        throw std::invalid_argument("unknown component");
    }

    Archetype* source = record_it->second.archetype;
    Archetype* target = nullptr;

    std::vector<ComponentId> target_signature{
        source->getComponentIds().begin(),
        source->getComponentIds().end()};

    if (source->component_edge.contains(component_id)) {
        auto& edge = source->component_edge.at(component_id);
        target = (edge.add != nullptr) ? edge.add : edge.remove;
    } else {
        const auto component_it = std::lower_bound(target_signature.begin(), target_signature.end(), component_id);
        const bool has_component = component_it != target_signature.end() && *component_it == component_id;

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

void World::normalizeSignature(std::vector<ComponentId>& signature) {
    std::sort(signature.begin(), signature.end());
    signature.erase(std::unique(signature.begin(), signature.end()), signature.end());
}

void World::validateSignature(std::span<const ComponentId> signature) {
    for (ComponentId component_id : signature) {
        if (!component_schemas.contains(component_id)) {
            throw std::invalid_argument("entity/archetype references an unknown component");
        }
    }
}

Archetype* World::findArchetype(const Signature& signature) {
    const auto& archetype_it = archetypes.find(signature);
    if (archetype_it != archetypes.end()) {
        return archetype_it->second.get();
    }
    return nullptr;
}

Archetype& World::getOrCreateArchetype(const std::vector<ComponentId>& signature) {
    if (Archetype* existing = findArchetype(signature)) {
        return *existing;
    }

    auto archetype_ptr = std::unique_ptr<Archetype>(new Archetype(signature, component_schemas));
    Archetype& archetype = *archetype_ptr;
    archetypes.emplace(signature, std::move(archetype_ptr));
    return archetype;
}

void World::removeUnusedArchetypes() {
    for (auto it = archetypes.begin(); it != archetypes.end();) {
        if (it->second->getEntities().empty()) {
            it = archetypes.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ecs
