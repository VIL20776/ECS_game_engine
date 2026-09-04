#pragma once
#include <expected>
#include <string>
#include <vector>

#include "component.hpp"

enum class LoaderError { parse_error, field_type_error };

struct ComponentDefinition {
    std::string name;
    std::vector<std::pair<std::string, ecs::FieldType>> fields;
};

class Loader {
    private:
    std::string project_path;

    public:
    Loader(const std::string& project_path);

    std::expected<std::vector<ComponentDefinition>, LoaderError> 
    readComponentFile(const std::string& file_path);

};
