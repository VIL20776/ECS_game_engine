#include "component.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ecs {

const ComponentField* 
ComponentSchema::findField(std::string_view field_name) const
{
    const auto it = std::find_if(fields.begin(), fields.end(), [&](const ComponentField& field) {
            return field.name == field_name;
            });
    return it == fields.end() ? nullptr : &*it;
}

bool ComponentSchema::validate() 
{
    if (id == InvalidComponentId || name.empty() || size == 0) {
        return false;
    }

    for (const ComponentField& field : fields) {
        if (field.name.empty() || field.size == 0 || field.alignment == 0) {
            return false;
        }
        if (field.offset > size || field.size > size - field.offset) {
            return false;
        }
    }
    return true;
}

std::size_t fieldTypeSize(FieldType type) 
{
    switch (type) {
        case FieldType::Bool:
            return 1;
        case FieldType::Int32:
        case FieldType::Float32:
            return 4;
        case FieldType::Bytes:
            return 0;
    }
    return 0;
}

std::span<const std::byte> getFieldBytes(
    const ComponentSchema& schema,
    std::span<const std::byte> component_data,
    std::string_view field_name) 
{
    if (component_data.size() != schema.size) {
        throw std::invalid_argument("component_data size does not match schema");
    }

    const ComponentField* field = schema.findField(field_name);
    if (field == nullptr) {
        throw std::out_of_range("field not found in component schema");
    }

    return component_data.subspan(field->offset, field->size);
}

std::span<std::byte> getFieldBytes(
    const ComponentSchema& schema,
    std::span<std::byte> component_data,
    std::string_view field_name) 
{
    if (component_data.size() != schema.size) {
        throw std::invalid_argument("component_data size does not match schema");
    }

    const ComponentField* field = schema.findField(field_name);
    if (field == nullptr) {
        throw std::out_of_range("field not found in component schema");
    }

    return component_data.subspan(field->offset, field->size);
}

template <typename T>
T readField(
    const ComponentSchema& schema,
    std::span<const std::byte> component_data,
    std::string_view field_name) 
{
    static_assert(std::is_trivially_copyable_v<T>);

    const auto bytes = getFieldBytes(schema, component_data, field_name);
    if (bytes.size() != sizeof(T)) {
        throw std::invalid_argument("requested type size does not match field size");
    }

    T value{};
    std::memcpy(&value, bytes.data(), sizeof(T));
    return value;
}

template <typename T>
void writeField(
    const ComponentSchema& schema,
    std::span<std::byte> component_data,
    std::string_view field_name,
    const T& value) 
{
    static_assert(std::is_trivially_copyable_v<T>);

    auto bytes = getFieldBytes(schema, component_data, field_name);
    if (bytes.size() != sizeof(T)) {
        throw std::invalid_argument("value size does not match field size");
    }

    std::memcpy(bytes.data(), &value, sizeof(T));
}

template
float readField(const ComponentSchema& schema, std::span<const std::byte> component_data, std::string_view field_name);

template
int readField(const ComponentSchema& schema, std::span<const std::byte> component_data, std::string_view field_name);

template
void writeField(const ComponentSchema& schema, std::span<std::byte> component_data, std::string_view field_name, const float& value);

template
void writeField(const ComponentSchema& schema, std::span<std::byte> component_data, std::string_view field_name, const int& value);


} // namespace ecs
