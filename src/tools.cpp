#include "tools.hpp"
#include "entity.hpp"
#include "imgui.h"
#include "world.hpp"
#include <format>

namespace app {

std::expected<void, ToolsError> ToolsManager::init(SDL_Window* window, SDL_GPUDevice* gpu_device) noexcept {
    if (window == nullptr || gpu_device == nullptr) {
        return std::unexpected(ToolsError::ImGuiContextFailed);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io_ = &ImGui::GetIO();
    io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplSDL3_InitForSDLGPU(window)) {
        return std::unexpected(ToolsError::SdlGuiBackendFailed);
    }

    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    if (!ImGui_ImplSDLGPU3_Init(&init_info)) {
        return std::unexpected(ToolsError::SdlGpuBackendFailed);
    }

    initialized_ = true;
    return {};
}

void ToolsManager::newFrame() noexcept {
    if (!initialized_) {
        return;
    }
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ToolsManager::entityViewerTool(ecs::World *ctx) noexcept {
    ImGui::Begin("Entity Viewer");

    for (ecs::EntityId eid = 0; eid < ctx->getEntityCount(); eid++) {
        auto& record = ctx->getEntityRecord(eid);

        std::string entity_label = std::format("Entity %d", eid);
        if (ImGui::TreeNode(entity_label.c_str())) {

            for (auto cid: record.archetype->getComponentIds()) {
                std::string component_name = ctx->getComponentSchema(cid).name;
                ImGui::Text(component_name.c_str());
            }

            ImGui::TreePop();
        }
    }
    
    ImGui::End();
}

void ToolsManager::renderFrame() noexcept {
    if (!initialized_) {
        return;
    }
    ImGui::Render();
}

void ToolsManager::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();
    io_ = nullptr;
    initialized_ = false;
}

} // namespace app
