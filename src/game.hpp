#pragma once

#include "system_runtime.hpp"
#include "world.hpp"

#include <expected>
#include <string>
#include <string_view>

#include "window.hpp"
#include "tools.hpp"

namespace game {

enum class GameError {
    ComponentLoadFailed,
    SystemLoadFailed,
    ToolsInitFailed,
    WindowInitFailed,
};

class Game {
public:
    Game();

    [[nodiscard]] std::expected<void, GameError> setup(std::string_view component_file, std::string_view systems_file);
    [[nodiscard]] int start() noexcept;
    [[nodiscard]] int input(SDL_Event* event, double delta_time);
    [[nodiscard]] int update(double delta_time);
    [[nodiscard]] int stop() noexcept;
    [[nodiscard]] int quit() noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

private:
    ecs::World world;
    ecs::lua::SystemRuntime systems;
    app::WindowManager window;
    app::ToolsManager tools;
    bool is_running{false};
};

} // namespace game
