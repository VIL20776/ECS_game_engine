#include "loader.hpp"

#include <cstdio>
#include <expected>
#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>
#include <unordered_map>

#include "component.hpp"

std::unordered_map<
    std::string, 
    ecs::FieldType
> type_map {
    {"f32", ecs::FieldType::Float32},
    {"i32", ecs::FieldType::Int32}
};

Loader::Loader(std::string_view project_path): project_path(project_path) {}

std::expected<std::vector<ComponentDefinition>, LoaderError> 
Loader::readComponentFile(std::string_view file_path)
{
    std::vector<ComponentDefinition> definitions;
    std::string component_file_path (this->project_path + file_path.data());
    toml::table tbl;
    try
    {
        tbl = toml::parse_file(component_file_path);
        std::cout << tbl << "\n";
    }
    catch (const toml::parse_error& err)
    {
        std::cerr << "Parsing failed:\n" << err << "\n";
        return std::unexpected(LoaderError::parse_error);
    }

    for (auto& item: tbl) {
        ComponentDefinition def;
        std::string name (item.first.str());
        def.name = name;
        for ( auto& field: *(item.second.as_table()) ) {
            std::string field_name (field.first.str());
            std::string field_type (field.second.value<std::string>().value());
            if (!type_map.contains(field_type)) {
                std::println(stderr, "Value '{:s}' for field '{:s}' is not a valid type", field_type.c_str(), field_name.c_str());
                return std::unexpected(LoaderError::field_type_error);
            }
            def.fields.emplace_back(field_name, type_map.at(field_type));
        }
        definitions.push_back(def);
    }

    return definitions;
}
