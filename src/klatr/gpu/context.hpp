#pragma once

#include <unordered_map>

#include <klatr/platform.hpp>

#include <vkom/instance.hpp>
#include <vkom/surface.hpp>
#include <vkom/adapter.hpp>
#include <vkom/device.hpp>
#include <vkom/swapchain.hpp>
#include <vkom/heap.hpp>
#include <vkom/resource.hpp>
#include <vkom/sampler.hpp>
#include <vkom/buffer.hpp>
#include <vkom/texture.hpp>
#include <vkom/fence.hpp>
#include <vkom/semaphore.hpp>
#include <vkom/descriptor.hpp>
#include <vkom/pipeline.hpp>
#include <vkom/queue.hpp>
#include <vkom/cmdencoder.hpp>
#include <vkom/cmdbatch.hpp>
#include <vkom/cmdpasses.hpp>

#include <SDL3/SDL.h>

namespace klatr {

namespace gpu {

#define KLATR_GPU_CONTEXT_SDL_WINDOW_METAL_VIEW_PROP "klatr.gpu.sdl_window.metal_view"

struct WindowData {
    SDL_Window* window = nullptr;
    vkom::ISurface* surface = nullptr;
    vkom::ISwapchain* swapchain = nullptr;
    vkom::IRenderTarget* renderTarget = nullptr;
};

struct Context {
    vkom::IInstance* instance = nullptr;
    vkom::IAdapter* adapter = nullptr;
    vkom::IDevice* device = nullptr;

    vkom::IQueue* audioComputeQueue = nullptr;
    vkom::IQueue* rasterGraphicsQueue = nullptr;
    vkom::IQueue* presentQueue = nullptr;

    std::unordered_map<SDL_Window*, WindowData> windows = {};

    Context(bool debug = false);
    ~Context();

    void vkomWSIInfoFromSDLWindow(SDL_Window* window, vkom::SurfaceWSIInfo* wsiInfo) const noexcept;
    bool getWindowData(SDL_Window* window, WindowData* data) noexcept;
};

}

}
