cmake_minimum_required(VERSION 3.28.3)

project(ecs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ECS_SOURCES
    src/world.cpp
    src/entity.cpp
    src/component.cpp
    src/archetype.cpp
)

set(IDE_SOURCES
    src/loader.cpp
)

include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

include(FetchContent)
FetchContent_Declare(
    LuaBridge
    GIT_REPOSITORY https://github.com/kunitoki/LuaBridge3.git
    GIT_TAG 3.0-rc12
)
FetchContent_MakeAvailable(LuaBridge)

option(BUILD_TESTS "Build Tests (Requires Catch2)" OFF)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    src/main.cpp
) 
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Catch2::Catch2WithMain
    tomlplusplus::tomlplusplus
)

if(BUILD_TESTS)
    find_package(Catch2 3 REQUIRED)

    add_executable(entity_component_test)
    target_sources(entity_component_test PRIVATE
        ${ECS_SOURCES}
        tests/entity_component_test.cpp
    )
    target_include_directories(entity_component_test PRIVATE src/)
    target_link_libraries(entity_component_test PRIVATE 
        Catch2::Catch2WithMain
    )

    add_executable(toml_lua_test)
    target_sources(toml_lua_test PRIVATE
        ${ECS_SOURCES}
        ${IDE_SOURCES}
        tests/toml_lua_test.cpp
    )
    target_include_directories(toml_lua_test PRIVATE src/) 
    target_link_libraries(toml_lua_test PRIVATE 
        Catch2::Catch2WithMain
        tomlplusplus::tomlplusplus
        LuaBridge
    )

    add_custom_command(
        TARGET toml_lua_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/tests/valid.toml
                ${CMAKE_CURRENT_BINARY_DIR}/valid.toml)

    include(CTest)
    include(Catch)
    catch_discover_tests(
        entity_component_test
        toml_lua_test
    )

endif(BUILD_TESTS)


