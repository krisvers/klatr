#include <klatr/gpu/context.hpp>

// #include <stdexcept>
#include <cassert>

namespace klatr {

namespace gpu {

Context::Context(bool debug) {
    assert(vkom::createInstance(debug, nullptr, &instance) == vkom::Result::Success);

    adapter = instance->enumerateAdapters(0);
    assert(adapter != nullptr);

    assert(adapter->createDevice(&device) == vkom::Result::Success);

    assert(device->acquireQueue(vkom::QUEUE_FAMILY_ANY, vkom::QueueFlags::Present, &presentQueue) == vkom::Result::Success);
    assert(device->acquireQueue(vkom::QUEUE_FAMILY_ANY, vkom::QueueFlags::Compute, &audioComputeQueue) == vkom::Result::Success);
    assert(device->acquireQueue(vkom::QUEUE_FAMILY_ANY, vkom::QueueFlags::Graphics, &rasterGraphicsQueue) == vkom::Result::Success);
}

Context::~Context() {
    device->waitIdle();

    for (auto pair : windows) {
        pair.second.renderTarget->release();
        pair.second.swapchain->release();
        pair.second.surface->release();

        #ifdef VKOM_PLATFORM_FAMILY_APPLE
        void* view = SDL_GetPointerProperty(SDL_GetWindowProperties(pair.second.window), KLATR_GPU_CONTEXT_SDL_WINDOW_METAL_VIEW_PROP, nullptr);
        if (view != nullptr) {
            SDL_Metal_DestroyView(view);
            SDL_SetPointerProperty(SDL_GetWindowProperties(pair.second.window), KLATR_GPU_CONTEXT_SDL_WINDOW_METAL_VIEW_PROP, view);
        }
        #endif
    }

    presentQueue->waitIdle();
    presentQueue->release();

    rasterGraphicsQueue->waitIdle();
    rasterGraphicsQueue->release();

    audioComputeQueue->waitIdle();
    audioComputeQueue->release();

    device->release();
    instance->release();
}

void Context::vkomWSIInfoFromSDLWindow(SDL_Window* window, vkom::SurfaceWSIInfo* wsiInfo) const noexcept {
    *wsiInfo = {};

    #ifdef VKOM_PLATFORM_FAMILY_NT
    wsiInfo->type = vkom::SurfaceWSIType::Win32;
    wsiInfo->windowHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    #elif defined(VKOM_PLATFORM_FAMILY_APPLE)
    void* view = SDL_GetPointerProperty(SDL_GetWindowProperties(window), KLATR_GPU_CONTEXT_SDL_WINDOW_METAL_VIEW_PROP, nullptr);
    if (view == nullptr) {
        view = SDL_Metal_CreateView(window);
        SDL_SetPointerProperty(SDL_GetWindowProperties(window), KLATR_GPU_CONTEXT_SDL_WINDOW_METAL_VIEW_PROP, view);
    }

    wsiInfo->type = vkom::SurfaceWSIType::Metal;
    wsiInfo->windowHandle = reinterpret_cast<uint64_t>(SDL_Metal_GetLayer(view));
    #elif defined(VKOM_PLATFORM_FAMILY_UNIX)
    const char* videoDriver = SDL_GetCurrentVideoDriver();
    if (std::strcmp(videoDriver, "x11") == 0) {
        wsiInfo->type = vkom::SurfaceSurfaceWSIType::Xlib;
        wsiInfo->windowHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, nullptr));
        wsiInfo->displayHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));
    } else if (std::strcmp(videoDriver, "wayland") == 0) {
        wsiInfo->type = vkom::SurfaceSurfaceWSIType::Wayland;
        wsiInfo->windowHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));
        wsiInfo->displayHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(_window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
    }
    #endif
}

bool Context::getWindowData(SDL_Window* window, WindowData* data) noexcept {
    auto it = windows.find(window);
    if (it != windows.end()) {
        *data = it->second;
        return false;
    }

    vkom::IWSIInstance* wsiInstance = instance->queryInterface<vkom::IWSIInstance>();
    if (wsiInstance == nullptr) {
        return false;
    }

    vkom::IWSIDevice* wsiDevice = device->queryInterface<vkom::IWSIDevice>();
    if (wsiDevice == nullptr) {
        return false;
    }

    vkom::SurfaceWSIInfo wsiInfo = {};
    vkomWSIInfoFromSDLWindow(window, &wsiInfo);

    if (wsiInstance->createSurface(&wsiInfo, &data->surface) != vkom::Result::Success) {
        return false;
    }

    vkom::SurfaceCapabilities capabilities = {};
    adapter->querySurfaceCapabilities(data->surface, &capabilities);

    vkom::SwapchainInfo swapchainInfo = {};
    swapchainInfo.backbufferCount = std::min(std::max(3u, capabilities.minBackbufferCount), capabilities.maxBackbufferCount);
    swapchainInfo.backbufferInfo.samplesPerTexel = 1;
    swapchainInfo.backbufferInfo.usage = vkom::TextureUsageFlags::TransferDestination | vkom::TextureUsageFlags::TransferSource;
    swapchainInfo.backbufferInfo.dimensions.extent = capabilities.currentExtent;
    swapchainInfo.backbufferInfo.dimensions.subresource.layers = 1;
    swapchainInfo.backbufferInfo.dimensions.subresource.mips = 1;
    swapchainInfo.preTransform = vkom::SurfaceTransformFlags::Identity;
    if ((capabilities.supportedCompositeAlpha & vkom::CompositeAlphaFlags::Opaque) != vkom::CompositeAlphaFlags::None) {
        swapchainInfo.compositeAlpha = vkom::CompositeAlphaFlags::Opaque;
    } else {
        swapchainInfo.compositeAlpha = vkom::CompositeAlphaFlags::PreMultiplied;
    }

    swapchainInfo.surfaceFormatBits = adapter->querySurfaceFormatBits(data->surface, vkom::Format::RGBA8UnsignedNormSRGB, vkom::ColorSpaceFlags::All) | adapter->querySurfaceFormatBits(data->surface, vkom::Format::RGBA8UnsignedNorm, vkom::ColorSpaceFlags::All) | adapter->querySurfaceFormatBits(data->surface, vkom::Format::BGRA8UnsignedNormSRGB, vkom::ColorSpaceFlags::All) | adapter->querySurfaceFormatBits(data->surface, vkom::Format::BGRA8UnsignedNorm, vkom::ColorSpaceFlags::All);
    swapchainInfo.presentModeFlags = vkom::PresentModeFlags::All;

    if (wsiDevice->createSwapchain(data->surface, &swapchainInfo, &data->swapchain) != vkom::Result::Success) {
        data->surface->release();
        return false;
    }

    vkom::TextureInfo renderTargetInfo = {};
    renderTargetInfo.format = vkom::Format::RGBA8UnsignedNorm;
    renderTargetInfo.samplesPerTexel = 1;
    renderTargetInfo.usage = vkom::TextureUsageFlags::RenderTarget | vkom::TextureUsageFlags::TransferSource;
    renderTargetInfo.dimensions.extent = capabilities.currentExtent;
    renderTargetInfo.dimensions.subresource.layers = 1;
    renderTargetInfo.dimensions.subresource.mips = 1;
    renderTargetInfo.location = vkom::MemoryLocationFlags::GPU;

    vkom::ITexture* renderTargetTexture;
    assert(device->defaultHeap()->createTexture(&renderTargetInfo, &renderTargetTexture) == vkom::Result::Success);

    data->renderTarget = renderTargetTexture->queryInterface<vkom::IRenderTarget>();
    assert(data->renderTarget != nullptr);

    return true;
}

}

}
