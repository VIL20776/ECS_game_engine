#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "component.hpp"
#include "world.hpp"

#include <LuaBridge/LuaBridge.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct lua_State;

namespace ecs::lua {

class ComponentView {
public:
    ComponentView(Archetype* archetype, ComponentSchema* schema, EntityId entity_id) noexcept;

    [[nodiscard]] ComponentId getId() const noexcept;
    [[nodiscard]] std::size_t getSize() const;

    [[nodiscard]] std::int64_t readInt(std::string field_name) const;
    [[nodiscard]] double readFloat(std::string field_name) const;
    [[nodiscard]] bool readBool(std::string field_name) const;

    void writeInt(std::string field_name, int value);
    void writeFloat(std::string field_name, double value);
    void writeBool(std::string field_name, bool value);

private:
    [[nodiscard]] std::span<const std::byte> raw() const;
    [[nodiscard]] std::span<std::byte> raw();
    void checkRange(std::size_t offset, std::size_t size) const;

    Archetype* archetype_{nullptr};
    ComponentSchema* schema_{nullptr};
    EntityId entity_id_{InvalidEntityId};
};

class SystemRuntime {
public:
    explicit SystemRuntime(World& world);
    ~SystemRuntime();

    SystemRuntime(const SystemRuntime&) = delete;
    SystemRuntime& operator=(const SystemRuntime&) = delete;

    void loadFile(const std::string& file_name);
    void update(double delta_time);

private:
    struct System {
        std::string name;
        std::vector<ComponentId> query;
        luabridge::LuaRef update;
    };

    void registerBindings();
    void registerSystem(const std::string& name, const luabridge::LuaRef& query, const luabridge::LuaRef& update);
    [[nodiscard]] std::vector<ComponentId> readQuery(const luabridge::LuaRef& query) const;
    [[nodiscard]] luabridge::LuaRef makeComponentTable(Archetype& archetype, EntityId entity_id, const System& system);

    World& world_;
    lua_State* lua_state_{nullptr};
    std::vector<System> systems_;
};

} // namespace ecs::lua
