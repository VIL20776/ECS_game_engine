#pragma once

#include "component.hpp"
#include "world.hpp"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <LuaBridge/LuaBridge.h>

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace ecs::lua {

enum class LuaRuntimeError {
    StateCreationFailed,
    SystemLoadFailed,
    QueryInvalid,
    SystemRegistrationFailed,
    SystemExecutionFailed,
};

class ComponentView {
public:
    ComponentView(Archetype* archetype, ComponentSchema* schema, EntityId entity_id) noexcept;

    [[nodiscard]] ComponentId getId() const noexcept;
    [[nodiscard]] std::size_t getSize() const;

    [[nodiscard]] std::int64_t readInt(std::string_view field_name) const;
    [[nodiscard]] double readFloat(std::string_view field_name) const;
    [[nodiscard]] bool readBool(std::string_view field_name) const;

    void writeInt(std::string_view field_name, int value);
    void writeFloat(std::string_view field_name, double value);
    void writeBool(std::string_view field_name, bool value);

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

    [[nodiscard]] std::expected<void, LuaRuntimeError> loadFile(std::string_view file_name);
    [[nodiscard]] std::expected<void, LuaRuntimeError> update(double delta_time);

private:
    struct System {
        std::string name;
        std::vector<ComponentId> query;
        luabridge::LuaRef update;
    };

    void registerBindings();
    [[nodiscard]] std::expected<void, LuaRuntimeError> registerSystem(
        const std::string& name,
        const luabridge::LuaRef& query,
        const luabridge::LuaRef& update);
    [[nodiscard]] std::expected<std::vector<ComponentId>, LuaRuntimeError> readQuery(const luabridge::LuaRef& query) const;
    [[nodiscard]] luabridge::LuaRef makeComponentTable(Archetype& archetype, EntityId entity_id, const System& system);

    World& world_;
    lua_State* lua_state_{nullptr};
    std::vector<System> systems_;
};

} // namespace ecs::lua
