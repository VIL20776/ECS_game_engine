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

void ToolsManager::entityViewerTool(ecs::World* ctx) noexcept {
    if (!initialized_ || ctx == nullptr) {
        return;
    }
    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 360.0f), ImGuiCond_Always);
    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
    ImGui::Begin("Entity Viewer");

    const auto entity_ids = ctx->getAllEntityIds();
    if (entity_ids.empty()) {
        ImGui::Text("No entities in the world");
    } else {
        ImGui::Text("Total entities: %zu", entity_ids.size());
        ImGui::Separator();

        for (const auto eid : entity_ids) {
            try {
                const auto& record = ctx->getEntityRecord(eid);
                const std::string entity_label = std::format("Entity {}", eid);

                if (ImGui::TreeNode(entity_label.c_str())) {
                    const auto component_ids = record.archetype->getComponentIds();
                    if (component_ids.empty()) {
                        ImGui::Text("  (no components)");
                    } else {
                        for (const auto cid : component_ids) {
                            try {
                                const auto& schema = ctx->getComponentSchema(cid);
                                ImGui::Bullet();
                                ImGui::Text("%s (id: %lu, size: %zu bytes)",
                                    schema.name.c_str(),
                                    cid,
                                    schema.size);
                            } catch (...) {
                                ImGui::Text("  <error reading component>");
                            }
                        }
                    }
                    ImGui::TreePop();
                }
            } catch (...) {
                ImGui::Text("Error reading entity %lu", eid);
            }
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
