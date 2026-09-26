#include "window.hpp"

#include <print>

namespace app {

std::expected<void, WindowError> WindowManager::init(std::string_view title, int width, int height) noexcept {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return std::unexpected(WindowError::SdlInitializationFailed);
    }

    window_ = SDL_CreateWindow(std::string{title}.c_str(), width, height, SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return std::unexpected(WindowError::WindowCreationFailed);
    }

    gpu_device_ = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL |
            SDL_GPU_SHADERFORMAT_METALLIB,
        true,
        nullptr);
    if (gpu_device_ == nullptr) {
        std::println("Error: SDL_CreateGPUDevice(): {}", SDL_GetError());
        return std::unexpected(WindowError::GpuDeviceFailed);
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device_, window_)) {
        std::println("Error: SDL_ClaimWindowForGPUDevice(): {}", SDL_GetError());
        return std::unexpected(WindowError::GpuWindowClaimFailed);
    }

    return {};
}

int WindowManager::handleEvent(const SDL_Event* event) const noexcept {
    if (event == nullptr) {
        return SDL_APP_CONTINUE;
    }
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

bool WindowManager::isValid() const noexcept {
    return window_ != nullptr && gpu_device_ != nullptr;
}

SDL_Window* WindowManager::nativeWindow() const noexcept {
    return window_;
}

SDL_GPUDevice* WindowManager::gpuDevice() const noexcept {
    return gpu_device_;
}

SDL_Window* WindowManager::window() const noexcept {
    return window_;
}

void WindowManager::shutdown() noexcept {
    if (gpu_device_ != nullptr) {
        SDL_WaitForGPUIdle(gpu_device_);
        SDL_ReleaseWindowFromGPUDevice(gpu_device_, window_);
        SDL_DestroyGPUDevice(gpu_device_);
        gpu_device_ = nullptr;
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

} // namespace app
