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

set(APP_SOURCES
    src/window.cpp
    src/tools.cpp
    src/game.cpp
    src/system_runtime.cpp
)

include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

FetchContent_Declare(
    sdl3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
)
FetchContent_MakeAvailable(sdl3)

include(ExternalProject)
set(LUABRIDGE3_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/luabridge3-src")
ExternalProject_Add(luabridge3_external
    GIT_REPOSITORY "https://github.com/kunitoki/LuaBridge3.git"
    GIT_TAG "3.0-rc12"
    GIT_SUBMODULES ""
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    SOURCE_DIR "${LUABRIDGE3_SOURCE_DIR}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
    UPDATE_DISCONNECTED TRUE
)

set(IMGUI_SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/imgui-src")
ExternalProject_Add(imgui_external
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG "v1.92.9b"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    SOURCE_DIR "${IMGUI_SOURCE_DIR}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
    UPDATE_DISCONNECTED TRUE
)

file(GLOB IMGUI_SOURCES
    ${IMGUI_SOURCE_DIR}/*.cpp
    ${IMGUI_SOURCE_DIR}/*.hpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.hpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.hpp
)

find_package(Lua 5.4 REQUIRED)

if(Lua_FOUND AND NOT TARGET Lua::Lua)
    add_library(Lua::Lua INTERFACE IMPORTED)
    set_target_properties(
        Lua::Lua
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
    )
endif()

add_library(LuaBridge3 INTERFACE)
add_library(LuaBridge3::LuaBridge ALIAS LuaBridge3)
add_dependencies(LuaBridge3 luabridge3_external)

target_include_directories(LuaBridge3
    INTERFACE
    "${LUABRIDGE3_SOURCE_DIR}/Distribution"
    "${LUABRIDGE3_SOURCE_DIR}/Source"
)
target_link_libraries(LuaBridge3 INTERFACE Lua::Lua)

option(BUILD_TESTS "Build Tests (Requires Catch2)" OFF)

add_executable(${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} imgui_external)

target_sources(${PROJECT_NAME} PRIVATE
    ${ECS_SOURCES}
    ${APP_SOURCES}
    ${IMGUI_SOURCES}
    src/main.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    ${SDL3_SOURCE_DIR}/include
    ${IMGUI_SOURCE_DIR}
    ${IMGUI_SOURCE_DIR}/backends
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Lua::Lua
    tomlplusplus::tomlplusplus
    LuaBridge3::LuaBridge
    SDL3::SDL3
)

if(BUILD_TESTS)
    find_package(Catch2 3 REQUIRED)

    add_executable(entity_component_test)
    target_sources(entity_component_test PRIVATE
        ${IMGUI_SOURCES}
        ${ECS_SOURCES}
        ${APP_SOURCES}
        tests/entity_component_test.cpp
    )
    target_include_directories(entity_component_test PRIVATE
        src/
        ${SDL3_SOURCE_DIR}/include
        ${IMGUI_SOURCE_DIR}
        ${IMGUI_SOURCE_DIR}/backends
    )
    target_link_libraries(entity_component_test PRIVATE
        Lua::Lua
        tomlplusplus::tomlplusplus
        LuaBridge3::LuaBridge
        SDL3::SDL3
        Catch2::Catch2WithMain
    )

    add_executable(toml_lua_test)
    target_sources(toml_lua_test PRIVATE
        ${IMGUI_SOURCES}
        ${ECS_SOURCES}
        ${APP_SOURCES}
        tests/toml_lua_test.cpp
    )
    target_include_directories(toml_lua_test PRIVATE
        src/
        ${SDL3_SOURCE_DIR}/include
        ${IMGUI_SOURCE_DIR}
        ${IMGUI_SOURCE_DIR}/backends
    )
    target_link_libraries(toml_lua_test PRIVATE
        Lua::Lua
        tomlplusplus::tomlplusplus
        LuaBridge3::LuaBridge
        SDL3::SDL3
        Catch2::Catch2WithMain
    )

    include(CTest)
    include(Catch)
    catch_discover_tests(entity_component_test toml_lua_test)
endif()
