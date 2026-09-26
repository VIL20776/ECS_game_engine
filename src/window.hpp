#pragma once

#include <expected>
#include <string_view>

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

namespace app {

enum class WindowError {
    SdlInitializationFailed,
    WindowCreationFailed,
    GpuDeviceFailed,
    GpuWindowClaimFailed,
};

class WindowManager {
public:
    WindowManager() = default;

    [[nodiscard]] std::expected<void, WindowError> init(std::string_view title, int width, int height) noexcept;
    [[nodiscard]] int handleEvent(const SDL_Event* event) const noexcept;
    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] SDL_Window* nativeWindow() const noexcept;
    [[nodiscard]] SDL_GPUDevice* gpuDevice() const noexcept;
    [[nodiscard]] SDL_Window* window() const noexcept;

    void shutdown() noexcept;

private:
    SDL_Window* window_{nullptr};
    SDL_GPUDevice* gpu_device_{nullptr};
};

} // namespace app
