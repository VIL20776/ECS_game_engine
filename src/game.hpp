#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

#include "system_runtime.hpp"
#include "world.hpp"

class Game {
    private:
    ecs::World world;
    ecs::lua::SystemRuntime systems;

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_GPUDevice* gpu_device = nullptr;

    ImGuiIO* io;

    std::string component_file;
    std::string systems_file;

    bool is_running;

    public:
    Game();

    int setup(const std::string& component_file, const std::string& systems_file);
    int start();
    int input(SDL_Event *event, double delta_time);
    int update(double delta_time);
    int stop();
    int quit();

    bool isRunning();
};
