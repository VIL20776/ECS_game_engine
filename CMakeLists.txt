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

option(BUILD_TESTS "Build Tests (Requires Catch2)" OFF)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    src/main.cpp
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

    include(CTest)
    include(Catch)
    catch_discover_tests(
        entity_component_test
    )

endif(BUILD_TESTS)


