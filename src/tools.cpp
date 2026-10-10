#include "tools.hpp"

#include "component.hpp"
#include "entity.hpp"
#include "imgui.h"
#include "world.hpp"
#include <cstring>
#include <format>
#include <optional>

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

void ToolsManager::forwardEvent(SDL_Event* event) noexcept {
    if (!initialized_) {
        return;
    }
    ImGui_ImplSDL3_ProcessEvent(event);
}

void ToolsManager::newFrame() noexcept {
    if (!initialized_) {
        return;
    }
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

std::optional<ecs::EntityId> selected_eid;
std::optional<ecs::ComponentId> selected_cid;

void ToolsManager::entityViewerTool(ecs::World* ctx) noexcept {
    if (!initialized_ || ctx == nullptr) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
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

                ImGui::PushID(eid);
                if (ImGui::TreeNode(entity_label.c_str())) {
                    const auto component_ids = record.archetype->getComponentIds();
                    if (component_ids.empty()) {
                        ImGui::Text("  (no components)");
                    } else {
                        for (const auto cid : component_ids) {
                            try {
                                const auto& schema = ctx->getComponentSchema(cid);
                                const std::string component_label = std::format("{} (size: {} bytes)", schema.name.c_str(), schema.size);

                                ImGui::PushID(cid);
                                ImGui::Bullet();

                                if (ImGui::Selectable(component_label.c_str())) {
                                    selected_eid = eid;
                                    selected_cid = schema.id;
                                }
                                ImGui::PopID();
                            } catch (...) {
                                ImGui::Text("  <error reading component>");
                            }
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            } catch (...) {
                ImGui::Text("Error reading entity %lu", eid);
            }
        }
    }

    ImGui::End();
}

void ToolsManager::componentViewerTool(ecs::World *ctx) noexcept {
    if (!initialized_ || ctx == nullptr) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Component Viewer");


    if (!selected_eid.has_value() || !selected_cid.has_value()) {
        ImGui::Text("No entity selected");
    } else {
        const auto eid = selected_eid.value();
        const auto cid = selected_cid.value();

        const auto record = ctx->getEntityRecord(eid);
        const auto schema = ctx->getComponentSchema(cid);

        ImGui::Text("Entity %zu: %s", eid, schema.name.c_str());
        ImGui::Separator();

        const auto data = record.archetype->getRawComponent(eid, cid);
        for (const auto field : schema.fields) {
            try {
                std::string component_label;
                const auto field_data = ecs::getFieldBytes(schema, data, field.name);
                
                ImGui::Bullet();
                switch (field.type) {
                    case ecs::FieldType::Float32: 
                        {
                            float value;
                            std::memcpy(&value, field_data.data(), sizeof(float));
                            ImGui::Text("%s: %.2f (type: Float32)", field.name.c_str(), value);       
                        }
                        break;
                    case ecs::FieldType::Int32:
                        {
                            int value;
                            std::memcpy(&value, field_data.data(), sizeof(int));
                            ImGui::Text("%s: %x (type: Int32)", field.name.c_str(), value);       
                        }
                        break;
                    case ecs::FieldType::Bool:
                        {
                            bool value;
                            std::memcpy(&value, field_data.data(), sizeof(bool));
                            if (value) ImGui::Text("%s: true (type: Bool)", field.name.c_str());       
                            else ImGui::Text("%s: false (type: Bool)", field.name.c_str());       
                        }
                        break;
                    case ecs::FieldType::Bytes:
                        {
                            char value;
                            std::memcpy(&value, field_data.data(), sizeof(char));
                            ImGui::Text("%s: %x (type: Byte)", field.name.c_str(), value);       
                        }
                        break;
                }

            } catch (...) {
                ImGui::Text("  <error reading component>");
            }
        }
    }

    ImGui::End(); 
}

void ToolsManager::archetypeViewerTool(ecs::World *ctx) noexcept {
    if (!initialized_ || ctx == nullptr) {
        return;
    }
    
    ImGui::Begin("Archetype Viewer");
    const auto archetypes = ctx->query();

    if (archetypes.empty()) {
        ImGui::Text("No archetypes in the world");
    } else {
        ImGui::Text("Total archetypes: %zu", archetypes.size());
        ImGui::Separator();

        for (const auto arch : archetypes) {
            try {
                const auto components = arch->getComponentIds();
                const auto entities = arch->getEntities();

                std::string archetype_label;
                for (auto cid: components) {
                    archetype_label += ctx->getComponentSchema(cid).name + ",";
                }

                if (archetype_label.empty()) 
                    archetype_label = "  (no components)";
                else
                    archetype_label.pop_back();

                if (ImGui::TreeNode((archetype_label + "##").c_str())) {
                    if (entities.empty()) {
                        ImGui::Text("  (no entities)");
                    } else {
                        std::size_t i = 0;
                        for (const auto eid : entities) {
                            ImGui::Bullet();
                            ImGui::Text("[%lu] Entity %lu", i++, eid); 
                        }
                    }
                    ImGui::TreePop();
                }
            } catch (...) {
                ImGui::Text("Error reading archetype");
            }
        }
    }

    ImGui::End(); 
}

void ToolsManager::toolBar(FunctionCall start, FunctionCall stop, FunctionCall reload) noexcept {
    if (!initialized_) {
        return;
    } 
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
