#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace ecs {

class Archetype;

using EntityId = std::uint64_t;
using ComponentId = std::uint64_t;
using ArchetypeId = std::uint64_t;

inline constexpr EntityId InvalidEntityId = std::numeric_limits<EntityId>::max();
inline constexpr ComponentId InvalidComponentId = std::numeric_limits<ComponentId>::max();
// inline constexpr ArchetypeId InvalidArchetypeId = std::numeric_limits<ArchetypeId>::max();

// Internal record used by World to resolve an EntityId into an Archetype row.
// Kept as a simple POD so entity identity remains independent from storage.
struct EntityRecord {
    Archetype* archetype {nullptr};
    std::size_t row{0};
};

} // namespace ecs
