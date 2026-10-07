#pragma once

#include "system_runtime.hpp"
#include "world.hpp"

#include <expected>
#include <string>
#include <string_view>

#include "window.hpp"
#include "tools.hpp"

namespace app {

enum class GameError {
    ComponentLoadFailed,
    EntityLoadFailed,
    SystemLoadFailed,
    ToolsInitFailed,
    WindowInitFailed,
};

class Game {
public:
    Game();

    [[nodiscard]] std::expected<void, GameError> 
    setup(std::string_view component_file, std::string_view entity_file, std::string_view systems_file);

    [[nodiscard]] int input(SDL_Event* event, double delta_time);
    [[nodiscard]] int update(double delta_time);
    [[nodiscard]] int render();
    void start() noexcept;
    void stop() noexcept;
    void reload() noexcept;
    [[nodiscard]] int quit() noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

private:
    void renderSDLComponents() noexcept;

    ecs::World world;
    ecs::lua::SystemRuntime systems;
    app::WindowManager window;
    app::ToolsManager tools;

    std::string component_file, entity_file, systems_file;

    bool is_running{false};
};

} // namespace game
