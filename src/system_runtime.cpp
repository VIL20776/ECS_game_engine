#include "system_runtime.hpp"
#include "component.hpp"
#include <print>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <LuaBridge/LuaBridge.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

namespace ecs::lua {
namespace {

template <typename T>
T readValue(std::span<const std::byte> data, std::size_t offset) {
    if (offset > data.size() || sizeof(T) > data.size() - offset) {
        throw std::out_of_range("component read is outside the raw data range");
    }
    T value{};
    std::memcpy(&value, data.data() + offset, sizeof(T));
    return value;
}

template <typename T>
void writeValue(std::span<std::byte> data, std::size_t offset, T value) {
    if (offset > data.size() || sizeof(T) > data.size() - offset) {
        throw std::out_of_range("component write is outside the raw data range");
    }
    std::memcpy(data.data() + offset, &value, sizeof(T));
}

} // namespace

ComponentView::ComponentView(Archetype* archetype, ComponentSchema* schema, EntityId entity_id) noexcept
    : archetype_(archetype), schema_(schema), entity_id_(entity_id) {}

ComponentId ComponentView::getId() const noexcept {
    return schema_->id;
}

std::size_t ComponentView::getSize() const {
    return raw().size();
}

std::span<const std::byte> ComponentView::raw() const {
    if (archetype_ == nullptr) {
        throw std::logic_error("invalid component view");
    }
    return archetype_->getRawComponent(entity_id_, schema_->id);
}

std::span<std::byte> ComponentView::raw() {
    if (archetype_ == nullptr) {
        throw std::logic_error("invalid component view");
    }
    return archetype_->getRawComponent(entity_id_, schema_->id);
}

void ComponentView::checkRange(std::size_t offset, std::size_t size) const {
    const auto data = raw();
    if (offset > data.size() || size > data.size() - offset) {
        throw std::out_of_range("component access is outside the raw data range");
    }
}

std::int64_t ComponentView::readInt(std::string field_name) const {
    auto* field = schema_->findField(field_name);
    return readValue<std::int32_t>(raw(), field->offset);
}

double ComponentView::readFloat(std::string field_name) const {
    std::println("Reading float");
    auto* field = schema_->findField(field_name);
    return static_cast<double>(readValue<float>(raw(), field->offset));
}

bool ComponentView::readBool(std::string field_name) const {
    auto* field = schema_->findField(field_name);
    return readValue<std::uint8_t>(raw(), field->offset) != 0;
}

void ComponentView::writeInt(std::string field_name, int value) {
    auto* field = schema_->findField(field_name);
    writeValue(raw(), field->offset, static_cast<std::int32_t>(value));
}

void ComponentView::writeFloat(std::string field_name, double value) {
    std::println("writing float");
    auto* field = schema_->findField(field_name);
    writeValue(raw(), field->offset, static_cast<float>(value));
}

void ComponentView::writeBool(std::string field_name, bool value) {
    auto* field = schema_->findField(field_name);
    writeValue(raw(), field->offset, static_cast<std::uint8_t>(value ? 1 : 0));
}

SystemRuntime::SystemRuntime(World& world) : world_(world), lua_state_(luaL_newstate()) {
    if (lua_state_ == nullptr) {
        throw std::runtime_error("could not create Lua state");
    }
    luaL_openlibs(lua_state_);
    registerBindings();
}

SystemRuntime::~SystemRuntime() {
    // LuaRef objects must be released before their lua_State is closed.
    systems_.clear();
    if (lua_state_ != nullptr) {
        lua_close(lua_state_);
    }
}

void SystemRuntime::registerBindings() {
    luabridge::getGlobalNamespace(lua_state_)
        .beginNamespace("ecs")
            .beginClass<ComponentView>("ComponentView")
                .addProperty("id", &ComponentView::getId)
                .addProperty("size", &ComponentView::getSize)
                .addFunction("readInt", &ComponentView::readInt)
                .addFunction("readFloat", &ComponentView::readFloat)
                .addFunction("readBool", &ComponentView::readBool)
                .addFunction("writeInt", &ComponentView::writeInt)
                .addFunction("writeFloat", &ComponentView::writeFloat)
                .addFunction("writeBool", &ComponentView::writeBool)
            .endClass()
            .addFunction("system", [this](const std::string& name, const luabridge::LuaRef& query, const luabridge::LuaRef& update) {
                registerSystem(name, query, update);
            })
        .endNamespace();
}

std::vector<ComponentId> SystemRuntime::readQuery(const luabridge::LuaRef& query) const {
    if (!query.isTable()) {
        throw std::invalid_argument("ecs.system query must be a Lua table");
    }

    std::vector<ComponentId> result;
    query.push(lua_state_);
    const int table_index = lua_absindex(lua_state_, -1);
    const lua_Integer count = static_cast<lua_Integer>(lua_rawlen(lua_state_, table_index));

    result.reserve(static_cast<std::size_t>(count));
    for (lua_Integer index = 1; index <= count; ++index) {
        lua_rawgeti(lua_state_, table_index, index);
        if (!lua_isstring(lua_state_, -1)) {
            lua_pop(lua_state_, 2);
            throw std::invalid_argument("ecs.system query entries must be integer ComponentId values");
        }

        const auto name = lua_tostring(lua_state_, -1);
        //  std::println("{}", name);
        lua_pop(lua_state_, 1);
        if (name == nullptr) {
            lua_pop(lua_state_, 1);
            throw std::invalid_argument("Component name cannot be empty");
        }
        result.push_back(static_cast<ComponentId>(world_.getComponentSchema(name).id));
    }
    lua_pop(lua_state_, 1);

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

void SystemRuntime::registerSystem(
    const std::string& name,
    const luabridge::LuaRef& query,
    const luabridge::LuaRef& update) {
    if (name.empty()) {
        throw std::invalid_argument("system name cannot be empty");
    }
    if (!update.isFunction()) {
        throw std::invalid_argument("ecs.system update must be a Lua function");
    }
    if (std::any_of(systems_.begin(), systems_.end(), [&](const System& system) { return system.name == name; })) {
        throw std::invalid_argument("a system named '" + name + "' is already registered");
    }

    systems_.push_back(System{name, readQuery(query), update});
}

void SystemRuntime::loadFile(const std::string& file_name) {
    if (luaL_loadfile(lua_state_, file_name.c_str()) != LUA_OK) {
        const std::string message = lua_tostring(lua_state_, -1);
        lua_pop(lua_state_, 1);
        throw std::runtime_error("could not load Lua file '" + file_name + "': " + message);
    }
    if (lua_pcall(lua_state_, 0, 0, 0) != LUA_OK) {
        const std::string message = lua_tostring(lua_state_, -1);
        lua_pop(lua_state_, 1);
        throw std::runtime_error("error executing Lua file '" + file_name + "': " + message);
    }
}

luabridge::LuaRef SystemRuntime::makeComponentTable(
    Archetype& archetype,
    EntityId entity_id,
    const System& system) 
{
    auto components = luabridge::newTable(lua_state_);
    for (ComponentId component_id : system.query) {
        auto& schema = world_.getComponentSchema(component_id);
        components[schema.name.c_str()] = ComponentView(&archetype, &schema, entity_id);
    }
    return components;
}

void SystemRuntime::update(double delta_time) {
    for (const System& system : systems_) {
        const auto archetypes = world_.query(system.query);
        for (Archetype* archetype : archetypes) {
            // Copy IDs because Lua code may indirectly cause structural ECS changes later.
            const auto entity_span = archetype->getEntities();
            const std::vector<EntityId> entities(entity_span.begin(), entity_span.end());

            for (EntityId entity_id : entities) {
                auto components = makeComponentTable(*archetype, entity_id, system);
                auto result = system.update(entity_id, components, delta_time);
                if (!result) {
                    throw std::runtime_error(
                        "Lua system '" + system.name + "' failed: " + result.message());
                }
            }
        }
    }
}

} // namespace ecs::lua
