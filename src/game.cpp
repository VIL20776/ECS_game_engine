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
    tools.entityViewerTool(&world);

    const auto update_result = systems.update(delta_time);
    if (!update_result.has_value()) {
        return SDL_APP_FAILURE;
    }

    tools.renderFrame();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(window.gpuDevice()); // Acquire a GPU command buffer

    SDL_GPUTexture* swapchain_texture;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window.window(), &swapchain_texture, nullptr, nullptr); // Acquire a swapchain texture

    if (swapchain_texture != nullptr && !is_minimized)
    {
        // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

        // Setup and start a render pass
        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = swapchain_texture;
        target_info.clear_color = SDL_FColor { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

        // Render ImGui
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

        SDL_EndGPURenderPass(render_pass);
    }

    // Submit the command buffer
    SDL_SubmitGPUCommandBuffer(command_buffer);

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
