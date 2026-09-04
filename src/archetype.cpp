#include "archetype.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ecs {

// ArchetypeId 
// Archetype::getId() { return archetype_id; }

std::span<const ComponentId> 
Archetype::getComponentIds() { return component_ids; }

std::span<const EntityId>
Archetype::getEntities() { return entities; }

bool Archetype::hasComponent(ComponentId component_id) {
    return component_columns.contains(component_id);
}

std::span<std::byte> Archetype::getRawComponent(
    EntityId entity_id,
    ComponentId component_id)
{
    const std::size_t row = findRow(entity_id);
    Column& column = getColumn(component_id);
    const std::size_t offset = row * column.element_size;
    return std::span<std::byte>{column.data}.subspan(offset, column.element_size);
}

void Archetype::setRawComponent(
    EntityId entity_id,
    ComponentId component_id,
    std::span<const std::byte> raw_data) 
{
    auto destination = getRawComponent(entity_id, component_id);
    if (destination.size() != raw_data.size()) {
        throw std::invalid_argument("raw component size does not match component schema");
    }
    std::memcpy(destination.data(), raw_data.data(), raw_data.size());
}

Archetype::Archetype(
    // ArchetypeId archetype_id_value,
    std::vector<ComponentId> component_ids_value,
    const std::unordered_map<ComponentId, ComponentSchema>& schemas)
    : component_ids(std::move(component_ids_value)) 
{
    std::sort(component_ids.begin(), component_ids.end());
    component_ids.erase(std::unique(component_ids.begin(), component_ids.end()), component_ids.end());

    for (ComponentId component_id : component_ids) {
        const auto schema_it = schemas.find(component_id);
        if (schema_it == schemas.end()) {
            throw std::invalid_argument("archetype references an unknown component");
        }
        component_columns.emplace(component_id, Column{schema_it->second.size, {}});
    }
}

std::size_t Archetype::findRow(EntityId entity_id) {
    const auto it = entity_rows.find(entity_id);
    if (it == entity_rows.end()) {
        throw std::out_of_range("entity does not belong to archetype");
    }
    return it->second;
}

Archetype::Column& Archetype::getColumn(ComponentId component_id) {
    const auto it = component_columns.find(component_id);
    if (it == component_columns.end()) {
        throw std::out_of_range("component does not belong to archetype");
    }
    return it->second;
}

std::size_t Archetype::appendEntity(EntityId entity_id) {
    if (entity_rows.contains(entity_id)) {
        throw std::invalid_argument("entity already belongs to archetype");
    }

    const std::size_t row = entities.size();
    entities.push_back(entity_id);
    entity_rows.emplace(entity_id, row);

    for (auto& [component_id, column] : component_columns) {
        (void)component_id;
        column.data.resize(column.data.size() + column.element_size, std::byte{0});
    }
    return row;
}

void Archetype::removeEntity(EntityId entity_id) {
    const std::size_t row = findRow(entity_id);
    const std::size_t last_row = entities.size() - 1;
    const EntityId moved_entity_id = entities[last_row];

    if (row != last_row) {
        entities[row] = moved_entity_id;
        entity_rows[moved_entity_id] = row;

        for (auto& [component_id, column] : component_columns) {
            (void)component_id;
            const std::size_t dst = row * column.element_size;
            const std::size_t src = last_row * column.element_size;
            std::memcpy(column.data.data() + dst, column.data.data() + src, column.element_size);
        }
    }

    entities.pop_back();
    entity_rows.erase(entity_id);
    for (auto& [component_id, column] : component_columns) {
        (void)component_id;
        column.data.resize(column.data.size() - column.element_size);
    }
}

} // namespace ecs
