#include "game.hpp"

#include <print>
#include <string>

namespace game {

Game::Game() : systems(world) {}

std::expected<void, GameError> Game::setup(std::string_view component_file, std::string_view systems_file) {
    const auto definitions = world.loadComponents(component_file);
    if (!definitions.has_value()) {
        return std::unexpected(GameError::ComponentLoadFailed);
    }

    for (const auto& def : definitions.value()) {
        const auto result = world.createComponent(def.name, def.fields);
        if (!result.has_value()) {
            return std::unexpected(GameError::ComponentLoadFailed);
        }
    }

    const auto system_result = systems.loadFile(systems_file);
    if (!system_result.has_value()) {
        return std::unexpected(GameError::SystemLoadFailed);
    }

    if (const auto result = window.init("example", 640, 480); !result.has_value()) {
        return std::unexpected(GameError::WindowInitFailed);
    }

    if (const auto result = tools.init(window.nativeWindow(), window.gpuDevice()); !result.has_value()) {
        return std::unexpected(GameError::ToolsInitFailed);
    }

    is_running = false;
    return {};
}

int Game::start() noexcept {
    is_running = true;
    return SDL_APP_CONTINUE;
}

int Game::input(SDL_Event* event, double delta_time) {
    (void)delta_time;
    if (event == nullptr) {
        return SDL_APP_CONTINUE;
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_COMMA) {
            std::println("Starting game");
            start();
        }
        if (event->key.key == SDLK_PERIOD) {
            std::println("Stoping game");
            stop();
        }
    }
    return SDL_APP_CONTINUE;
}

int Game::update(double delta_time) {
    tools.newFrame();

    ImGui::ShowDemoWindow();
    systems.update(delta_time);

    ImGui::Render();
    return SDL_APP_CONTINUE;
}

int Game::stop() noexcept {
    is_running = false;
    return SDL_APP_CONTINUE;
}

int Game::quit() noexcept {
    tools.shutdown();
    window.shutdown();
    return SDL_APP_SUCCESS;
}

bool Game::isRunning() const noexcept {
    return is_running;
}

} // namespace game
