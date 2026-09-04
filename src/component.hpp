#pragma once

#include "entity.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ecs {

enum class FieldType : std::uint8_t {
    Int32,
    Float32,
    Bool,
    Bytes
};

struct ComponentField {
    std::string name;
    FieldType type{FieldType::Bytes};
    std::size_t offset{0};
    std::size_t size{0};
    std::size_t alignment{1};
};

// Reflection metadata for a component. It deliberately does not depend on a
// user-defined C++ struct/class; component layouts are described explicitly.
struct ComponentSchema {
    ComponentId id{InvalidComponentId};
    std::string name;
    std::size_t size{0};
    std::vector<ComponentField> fields;

    const ComponentField* findField(std::string_view field_name) const;
    bool validate();
};

std::size_t fieldTypeSize(FieldType type);

std::span<const std::byte> getFieldBytes(
    const ComponentSchema& schema,
    std::span<const std::byte> component_data,
    std::string_view field_name);

std::span<std::byte> getFieldBytes(
    const ComponentSchema& schema,
    std::span<std::byte> component_data,
    std::string_view field_name); 

template <typename T>
T readField(
    const ComponentSchema& schema,
    std::span<const std::byte> component_data,
    std::string_view field_name);

template <typename T>
void writeField(
    const ComponentSchema& schema,
    std::span<std::byte> component_data,
    std::string_view field_name,
    const T& value);

} // namespace ecs
