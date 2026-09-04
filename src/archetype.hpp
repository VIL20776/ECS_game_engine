#pragma once

#include "entity.hpp"
#include "component.hpp"

#include <cstddef>
#include <cstring>
#include <span>
#include <unordered_map>
#include <vector>

namespace ecs {

using Signature = std::vector<ComponentId>;

class World;

class Archetype {
    private:
    friend class World;

    struct Column {
        std::size_t element_size{0};
        std::vector<std::byte> data;
    };

    struct Edge {
        Archetype* add {nullptr};
        Archetype* remove {nullptr};
    };

    struct Hash {
        std::size_t operator()(const Signature& set) const {
            std::size_t h = 0;
            for (auto e: set)
                h ^= std::hash<ComponentId>{}(e) + 0x9e3779b9 + (h << 6) + (h >> 2);

            return h;
        }
    };

    // ArchetypeId archetype_id{InvalidArchetypeId};
    std::vector<ComponentId> component_ids;
    std::vector<EntityId> entities;
    std::unordered_map<EntityId, std::size_t> entity_rows;
    std::unordered_map<ComponentId, Column> component_columns;
    std::unordered_map<ComponentId, Edge> component_edge;

    Archetype(
        // ArchetypeId archetype_id_value,
        std::vector<ComponentId> component_ids_value,
        const std::unordered_map<ComponentId, ComponentSchema>& schemas);

    std::size_t findRow(EntityId entity_id);

    Column& getColumn(ComponentId component_id);

    std::size_t appendEntity(EntityId entity_id);

    void removeEntity(EntityId entity_id);

    public:
    // ArchetypeId getId();

    std::span<const ComponentId> getComponentIds();

    std::span<const EntityId> getEntities();

    bool hasComponent(ComponentId component_id);

    std::span<std::byte> getRawComponent(
        EntityId entity_id,
        ComponentId component_id);

    void setRawComponent(
        EntityId entity_id,
        ComponentId component_id,
        std::span<const std::byte> raw_data);
};

} // namespace ecs
