#include "game.hpp"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "component.hpp"
#include "entity.hpp"
#include "world.hpp"

#include <cstring>
#include <print>
#include <string>

namespace app {

Game::Game() : systems(world) {}

std::expected<void, GameError> Game::setup(std::string_view component_file, std::string_view entity_file, std::string_view systems_file) {
    this->component_file = component_file;
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

    this->entity_file = entity_file;
    const auto entity_result = world.loadEntities(entity_file);
    if (!entity_result.has_value()) {
        return std::unexpected(GameError::EntityLoadFailed); 
    }

    this->systems_file = systems_file;
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

    tools.forwardEvent(event);
    return SDL_APP_CONTINUE;
}

int Game::update(double delta_time) {
    const auto update_result = systems.update(delta_time);
    if (!update_result.has_value()) {
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

int Game::render() {
    tools.newFrame();

    tools.entityViewerTool(&world);
    tools.componentViewerTool(&world);
    tools.archetypeViewerTool(&world);

    ImGui::Begin("Toolbar");
    if (ImGui::Button("Play")) {
        start();
    }

    if (ImGui::Button("Stop")) {
        stop();
    }

    if (ImGui::Button("Reload")) {
        reload();
    }
    ImGui::End();

    renderSDLComponents();

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

void Game::start() noexcept {
    const auto update_result = systems.setup();
    if (!update_result.has_value()) {
        return;
    }
    is_running = true;
}

void Game::reload() noexcept {
    this->world = ecs::World();
    // systems.reset(world);

    const auto definitions = world.loadComponents(component_file);
    if (!definitions.has_value()) {
        // return std::unexpected(GameError::ComponentLoadFailed);
    }

    for (const auto& def : definitions.value()) {
        const auto result = world.createComponent(def.name, def.fields);
        if (!result.has_value()) {
            // return std::unexpected(GameError::ComponentLoadFailed);
        }
    }

    const auto entity_result = world.loadEntities(entity_file);
    if (!entity_result.has_value()) {
        // return std::unexpected(GameError::EntityLoadFailed); 
    }

    const auto system_result = systems.loadFile(systems_file);
    if (!system_result.has_value()) {
        // return std::unexpected(GameError::SystemLoadFailed);
    }
    is_running = false;
}

void Game::stop() noexcept {
    is_running = false;
}

int Game::quit() noexcept {
    tools.shutdown();
    window.shutdown();
    return SDL_APP_SUCCESS;
}

bool Game::isRunning() const noexcept {
    return is_running;
}

void Game::renderSDLComponents() noexcept {
    // SDL Rects
    const auto rect_schema = world.getComponentSchema("Rectangle");
    const auto archetypes = world.query(std::vector<ecs::ComponentId>{rect_schema.id});

    for (auto* arch: archetypes) {
        SDL_FRect rect;
        for (auto eid: arch->getEntities()) {
            auto data = arch->getRawComponent(eid, rect_schema.id);

            auto data_x = ecs::getFieldBytes(rect_schema, data, "x");
            auto data_y = ecs::getFieldBytes(rect_schema, data, "y");
            auto data_h = ecs::getFieldBytes(rect_schema, data, "h");
            auto data_w = ecs::getFieldBytes(rect_schema, data, "w");

            std::memcpy(&rect.x, data_x.data(), sizeof(float));
            std::memcpy(&rect.y, data_y.data(), sizeof(float));
            std::memcpy(&rect.h, data_h.data(), sizeof(float));
            std::memcpy(&rect.w, data_w.data(), sizeof(float));

            const ImVec2 origin = ImGui::GetMainViewport()->Pos;
            ImGui::GetBackgroundDrawList()->AddRectFilled(
                    ImVec2(origin.x + rect.x,          origin.y + rect.y),
                    ImVec2(origin.x + rect.x + rect.w + 10, origin.y + rect.y + rect.h + 10),
                    IM_COL32(255, 100, 50, 255)
                    );
        }
    }
}

} // namespace game
