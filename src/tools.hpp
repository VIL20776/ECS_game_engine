#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#include "SDL3/SDL.h"
#include "world.hpp"

#include <expected>

namespace app {

enum class ToolsError {
    ImGuiContextFailed,
    SdlGuiBackendFailed,
    SdlGpuBackendFailed,
};

class ToolsManager {
public:
    ToolsManager() = default;

    [[nodiscard]] std::expected<void, ToolsError> init(SDL_Window* window, SDL_GPUDevice* gpu_device) noexcept;
    void newFrame() noexcept;
    void entityViewerTool(ecs::World* ctx) noexcept;
    void renderFrame() noexcept;
    void shutdown() noexcept;

private:
    ImGuiIO* io_{nullptr};
    bool initialized_{false};
};

} // namespace app
