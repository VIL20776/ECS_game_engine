#pragma once
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "component.hpp"
#include "world.hpp"

enum class LoaderError { parse_error, field_type_error };

struct ComponentDefinition {
    std::string name;
    std::vector<ecs::ComponentField> fields;
};

class Loader {
    private:
    ecs::World* world;
    std::string project_path;

    public:
    Loader(std::string_view project_path);

    std::expected<std::vector<ComponentDefinition>, LoaderError> 
    readComponentFile(std::string_view file_path);
};
