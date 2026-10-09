#include <klatr/gpu/context.hpp>

#include <miniaudio.h>

#include <SDL3/SDL.h>

#include <bitset>
#include <fstream>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <semaphore>

template<typename T, size_t N>
class BoundedQueue {
private:
    T* _data;

    size_t _producerOffset = 0;
    size_t _consumerOffset = 0;

    std::counting_semaphore<N> _full{0};
    std::counting_semaphore<N> _empty{N};
    std::mutex _lock = {};

public:
    BoundedQueue() {
        _data = new T[N];
    }

    ~BoundedQueue() {
        delete[] _data;
    }

    T const* data() const {
        return _data;
    }

    T* data() {
        return _data;
    }

    size_t capacity() const {
        return N;
    }

    size_t producerOffset() const {
        return _producerOffset;
    }

    size_t consumerOffset() const {
        return _consumerOffset;
    }

    bool produce(size_t count, T const* data, bool blocking = true) {
        if (blocking) {
            _empty.acquire();
        } else {
            if (!_empty.try_acquire()) {
                return false;
            }
        }

        {
            std::lock_guard<std::mutex> guard(_lock);

            size_t copyRegions = ((_producerOffset % N) + count + N - 1) / N;
            assert(copyRegions <= 2);

            size_t firstCopyCount = std::min(N - (_producerOffset % N), count);
            size_t secondCopyCount = count - firstCopyCount;

            std::memcpy(&_data[_producerOffset % N], data, firstCopyCount * sizeof(T));
            if (secondCopyCount != 0) {
                std::memcpy(&_data[0], &data[firstCopyCount], secondCopyCount * sizeof(T));
            }

            _producerOffset += count;
        }

        _full.release();
        return true;
    }

    size_t consume(size_t count, T* data, bool blocking = true) {
        if (blocking) {
            _full.acquire();
        } else {
            if (!_full.try_acquire()) {
                return 0;
            }
        }

        count = std::min(std::min(_producerOffset - _consumerOffset, N), count);

        {
            std::lock_guard<std::mutex> guard(_lock);

            size_t copyRegions = ((_consumerOffset % N) + count + N - 1) / N;
            assert(copyRegions <= 2);

            size_t firstCopyCount = std::min(N - (_consumerOffset % N), count);
            size_t secondCopyCount = count - firstCopyCount;

            std::memcpy(&data[0], &_data[_consumerOffset % N], firstCopyCount * sizeof(T));
            if (secondCopyCount != 0) {
                std::memcpy(&data[firstCopyCount], &_data[0], secondCopyCount * sizeof(T));
            }

            _consumerOffset += count;
        }

        _empty.release();
        return count;
    }
};

enum class PassResourceMode {
    Consume = 1,
    Produce = 2,
};

enum class PassResourceFormat {
    Float = 1,
    Int32 = 2,
};

enum class PassResourceAddressingType {
    SSBO = 1,
    DeviceAddress = 2,
};

struct PassResourceAddressingInfo {
    PassResourceAddressingType type;
    uint32_t bindingOrSlot;
    uint32_t set;
    char name[128];
};

struct PassResourceBounds {
    float lower;
    float upper;
};

struct PassSlotResourceInfo {
    uint32_t count;
    PassResourceAddressingInfo addressing;
    PassResourceMode mode;
    PassResourceFormat format;
    PassResourceBounds defaultBounds;
};

struct PassSlotTileInfo {
    uint32_t width;
    uint32_t height;
    uint32_t binding;
    uint32_t set;
    char name[128];
};

class IPass {
public:
    virtual bool enumerateResourceSlots(uint32_t id, PassSlotResourceInfo* info) = 0;
    virtual bool enumerateTileSlots(uint32_t id, PassSlotTileInfo* info) = 0;

    virtual bool prepare() = 0;

    virtual bool bindResourceSSBO(uint32_t binding, uint32_t set, vkom::IBufferView* view, PassResourceBounds const* bounds) = 0;
    virtual bool bindResourceDeviceAddress(uint32_t slot, vkom::IDeviceAddressBuffer* buffer, PassResourceBounds const* bounds) = 0;
    virtual bool bindResource(const char* name, vkom::IBufferView* ssboView, vkom::IDeviceAddressBuffer* dabo, PassResourceBounds const* bounds) = 0;

    virtual bool bindConstantSSBO(uint32_t binding, uint32_t set, float f, int32_t i) = 0;
    virtual bool bindConstantDeviceAddress(uint32_t slot, float f, int32_t i) = 0;
    virtual bool bindConstant(const char* name, float f, int32_t i) = 0;

    virtual bool bindTile(const char* name, vkom::ITextureView* view) = 0;
    virtual bool bindTileBindingSet(uint32_t binding, uint32_t set, vkom::ITextureView* view) = 0;

    virtual bool execute(vkom::ICommandEncoder* encoder) = 0;
};

#define INTERNAL_PLAYBACK_BUFFER_SAMPLE_COUNT 44100 * 10
#define INTERNAL_AUDIO_BUFFER_FRAME_COUNT 16384

//struct UniformAudioBufferDescriptor {
//    uint32_t count;
//    uint32_t padding0;
//    float bounds[2]; /* note: if bounds[0] == bounds[1] this is treated as a constant */
//};

struct Uniforms {
    uint64_t globalID;
    float globalTime;
    uint32_t dispatchWidth;
    uint32_t sampleRate;
    float inverseSampleRate;
};

struct PushConstantAudioBufferDescriptor {
    uint64_t address;
    uint32_t count;
    uint32_t padding0;
    float bounds[2]; /* note: if bounds[0] == bounds[1] this is treated as a constant */
};

struct PushConstants {
    PushConstantAudioBufferDescriptor buffer;
};

using PlaybackBuffer = BoundedQueue<float, INTERNAL_PLAYBACK_BUFFER_SAMPLE_COUNT>;

/* adapted from old test code from krisvers/vkom */
std::vector<uint32_t> loadFile(const char* path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.good()) {
        return {};
    }

    size_t byteSize = in.tellg();
    if (byteSize % 4 != 0) {
        in.close();
        return {};
    }

    std::vector<uint32_t> code(byteSize / 4);
    in.seekg(0, std::ios::beg);

    if (!in.read(reinterpret_cast<char*>(&code[0]), byteSize)) {
        in.close();
        return {};
    }

    in.close();
    return code;
}

struct ModuleInfo {
    std::filesystem::file_time_type lastModified;
    std::vector<uint32_t> spirv;
    vkom::IShaderModule* shader;
};

vkom::IShaderModule* loadModule(ModuleInfo& info, vkom::IDevice* device, const char* path, std::filesystem::file_time_type previousLastModified = std::filesystem::file_time_type::min()) {
    info = {};

    if (!std::filesystem::exists(path)) {
        return nullptr;
    }

    info.lastModified = std::filesystem::last_write_time(path);
    if (info.lastModified <= previousLastModified) {
        return nullptr;
    }

    info.spirv = loadFile(path);
    if (info.spirv.empty()) {
        return nullptr;
    }

    vkom::ShaderModuleInfo moduleShaderInfo = {};
    moduleShaderInfo.length = info.spirv.size();
    moduleShaderInfo.spirv = &info.spirv[0];

    if (device->createShaderModule(&moduleShaderInfo, &info.shader) != vkom::Result::Success) {
        return nullptr;
    }

    return info.shader;
}

vkom::IComputePipeline* loadModuleAndPipeline(ModuleInfo& moduleInfo, vkom::IDevice* device, vkom::IPipelineLayout* layout, const char* path, std::filesystem::file_time_type previousLastModified = std::filesystem::file_time_type::min()) {
    if (loadModule(moduleInfo, device, path, previousLastModified) == nullptr) {
        return nullptr;
    }

    vkom::ComputePipelineInfo modulePipelineInfo = {};
    modulePipelineInfo.shaderInfo.shader = moduleInfo.shader;
    modulePipelineInfo.shaderInfo.stage = vkom::ShaderStageFlags::Compute;
    modulePipelineInfo.shaderInfo.entry = "module";

    vkom::IComputePipeline* modulePipeline;
    if (device->createComputePipeline(&modulePipelineInfo, nullptr, layout, &modulePipeline) != vkom::Result::Success) {
        return nullptr;
    }

    return modulePipeline;
}

struct GeneralPassPipelineData {
    vkom::IPipelineLayout* layout;
    vkom::IComputePipeline* pipeline;
};

struct GeneralPassDescriptorSetLayoutData {
    uint32_t set;
    vkom::IDescriptorSetLayout* layout;
};

struct GeneralPassDispatchDescriptorSetData {
    uint32_t setID;
    vkom::IDescriptorSet* set;
};

struct GeneralPassDispatchData {
    vkom::IFence* fence;
    std::vector<GeneralPassDispatchDescriptorSetData> sets;
};

struct GeneralPassResourceBindingData {
    vkom::IBufferView* ssboView = nullptr;
    vkom::IDeviceAddressBuffer* dabo = nullptr;
    PassResourceBounds bounds;
    float f = 0.0f;
    int32_t i = 0;
};

template<size_t DispatchesInFlight, bool blocking = true>
class GeneralPass : public IPass {
private:
    klatr::gpu::Context const& _context;
    GeneralPassPipelineData _pipelineData;
    std::vector<PassSlotResourceInfo> _resourceInfos = {};
    std::vector<PassSlotTileInfo> _tileInfos = {};

    bool _isDescriptorless = false;
    uint32_t _dispatchID = 0;
    std::array<GeneralPassDispatchData, DispatchesInFlight> _dispatchDatas = {};

    std::vector<GeneralPassResourceBindingData> _resourceBindings = {};
    std::vector<vkom::ITextureView*> _tileBindings = {};

public:
    GeneralPass(klatr::gpu::Context const& context, GeneralPassPipelineData const& pipelineData, std::vector<PassSlotResourceInfo> const& resourceInfos, std::vector<PassSlotTileInfo> const& tileInfos) : _context(context), _pipelineData(pipelineData), _resourceInfos(resourceInfos), _tileInfos(tileInfos), _resourceBindings(_resourceInfos.size(), nullptr), _tileBindings(_tileInfos.size(), nullptr) {
        _pipelineData.layout->retain();
        _pipelineData.pipeline->retain();

        _isDescriptorless = _tileInfos.empty();

        for (PassSlotResourceInfo const& info : _resourceInfos) {
            if (info.addressing.type == PassResourceAddressingType::SSBO) {
                _isDescriptorless = false;
                break;
            }
        }
    }

    ~GeneralPass() {
        for (GeneralPassDispatchData const& dispatch : _dispatchDatas) {
            if (dispatch.fence != nullptr) {
                dispatch.fence->release();
            }

            for (GeneralPassDispatchDescriptorSetData const& set : dispatch.sets) {
                if (set.set != nullptr) {
                    set.set->release();
                }
            }
        }

        for (GeneralPassResourceBindingData const& binding : _resourceBindings) {
            if (binding.ssboView != nullptr) {
                binding.ssboView->release();
            }

            if (binding.dabo != nullptr) {
                binding.dabo->release();
            }
        }

        for (vkom::ITextureView* tile : _tileBindings) {
            if (tile != nullptr) {
                tile->release();
            }
        }

        _pipelineData.layout->release();
        _pipelineData.pipeline->release();
    }

    bool enumerateResourceSlots(uint32_t id, PassSlotResourceInfo* info) override {
        if (id >= _resourceInfos.size()) {
            return false;
        }

        *info = _resourceInfos[id];
        return true;
    }

    bool enumerateTileSlots(uint32_t id, PassSlotTileInfo* info) override {
        if (id >= _tileInfos.size()) {
            return false;
        }

        *info = _tileInfos[id];
        return true;
    }

    bool prepare() override {
        return true;
    }

    bool bindResourceSSBO(uint32_t binding, uint32_t set, vkom::IBufferView* view, PassResourceBounds const* bounds) override {
        vkom::IBuffer* buffer = view->parent<vkom::IBuffer>();
        if (buffer == nullptr) {
            return false;
        }

        vkom::IStorageBuffer* ssbo = buffer->queryInterface<vkom::IStorageBuffer>();
        if (ssbo == nullptr) {
            return false;
        }

        for (uint32_t i = 0; i < _resourceInfos.size(); i += 1) {
            if (_resourceInfos[i].addressing.type != PassResourceAddressingType::SSBO) {
                continue;
            }

            if (_resourceInfos[i].addressing.bindingOrSlot != binding || _resourceInfos[i].addressing.set != set) {
                continue;
            }

            if (_resourceBindings[i].ssboView != nullptr) {
                _resourceBindings[i].ssboView->release();
            }

            if (_resourceBindings[i].dabo != nullptr) {
                _resourceBindings[i].dabo->release();
            }

            _resourceBindings[i].dabo = nullptr;
            _resourceBindings[i].ssboView = view;
            view->retain();

            return true;
        }

        return false;
    }

    bool bindResourceDeviceAddress(uint32_t slot, vkom::IDeviceAddressBuffer* buffer, PassResourceBounds const* bounds) override {
        for (uint32_t i = 0; i < _resourceInfos.size(); i += 1) {
            if (_resourceInfos[i].addressing.type != PassResourceAddressingType::DeviceAddress) {
                continue;
            }

            if (_resourceInfos[i].addressing.bindingOrSlot != slot) {
                continue;
            }

            if (_resourceBindings[i].ssboView != nullptr) {
                _resourceBindings[i].ssboView->release();
            }

            if (_resourceBindings[i].dabo != nullptr) {
                _resourceBindings[i].dabo->release();
            }

            _resourceBindings[i].ssboView = nullptr;
            _resourceBindings[i].dabo = buffer;
            buffer->retain();

            return true;
        }

        return false;
    }

    bool bindResource(const char* name, vkom::IBufferView* ssboView, vkom::IDeviceAddressBuffer* dabo, PassResourceBounds const* bounds) override {
        if (ssboView != nullptr) {
            vkom::IBuffer* buffer = ssboView->parent<vkom::IBuffer>();
            if (buffer == nullptr) {
                return false;
            }

            vkom::IStorageBuffer* ssbo = buffer->queryInterface<vkom::IStorageBuffer>();
            if (ssbo == nullptr) {
                return false;
            }
        }

        for (uint32_t i = 0; i < _resourceInfos.size(); i += 1) {
            if (std::strcmp(_resourceInfos[i].addressing.name, name) != 0) {
                continue;
            }

            if (_resourceInfos[i].addressing.type == PassResourceAddressingType::SSBO && ssboView == nullptr) {
                return false;
            }

            if (_resourceInfos[i].addressing.type == PassResourceAddressingType::DeviceAddress && dabo == nullptr) {
                return false;
            }

            if (_resourceBindings[i].ssboView != nullptr) {
                _resourceBindings[i].ssboView->release();
            }

            if (_resourceBindings[i].dabo != nullptr) {
                _resourceBindings[i].dabo->release();
            }
            
            if (_resourceInfos[i].addressing.type == PassResourceAddressingType::SSBO) {
                _resourceBindings[i].dabo = nullptr;
                _resourceBindings[i].ssboView = ssboView;
                ssboView->retain();
            } else if (_resourceInfos[i].addressing.type == PassResourceAddressingType::DeviceAddress) {
                _resourceBindings[i].ssboView = nullptr;
                _resourceBindings[i].dabo = dabo;
                dabo->retain();
            }

            if (bounds != nullptr) {
                _resourceBindings[i].bounds = *bounds;
            } else {
                _resourceBindings[i].bounds = _resourceInfos[i].defaultBounds;
            }

            return true;
        }

        return false;
    }

    bool bindConstantSSBO(uint32_t binding, uint32_t set, float f, int32_t i) override {
        for (uint32_t i = 0; i < _resourceInfos.size(); i += 1) {
            if (_resourceInfos[i].addressing.type != PassResourceAddressingType::SSBO) {
                continue;
            }

            if (_resourceInfos[i].addressing.bindingOrSlot != binding || _resourceInfos[i].addressing.set != set) {
                continue;
            }

            if (_resourceBindings[i].ssboView != nullptr) {
                _resourceBindings[i].ssboView->release();
            }

            if (_resourceBindings[i].dabo != nullptr) {
                _resourceBindings[i].dabo->release();
            }

            _resourceBindings[i] = {};
            _resourceBindings[i].f = f;
            _resourceBindings[i].i = i;

            return true;
        }

        return false;
    }

    bool bindConstantDeviceAddress(uint32_t slot, float f, int32_t i) override {
        for (uint32_t i = 0; i < _resourceInfos.size(); i += 1) {
            if (_resourceInfos[i].addressing.type != PassResourceAddressingType::DeviceAddress) {
                continue;
            }

            if (_resourceInfos[i].addressing.bindingOrSlot != slot) {
                continue;
            }

            if (_resourceBindings[i].ssboView != nullptr) {
                _resourceBindings[i].ssboView->release();
            }

            if (_resourceBindings[i].dabo != nullptr) {
                _resourceBindings[i].dabo->release();
            }

            _resourceBindings[i] = {};
            _resourceBindings[i].f = f;
            _resourceBindings[i].i = i;

            return true;
        }

        return false;
    }

    bool bindConstant(const char* name, float f, int32_t i) override {
        for (uint32_t i = 0; i < _resourceInfos.size(); i += 1) {
            if (std::strcmp(_resourceInfos[i].addressing.name, name) != 0) {
                continue;
            }

            if (_resourceBindings[i].ssboView != nullptr) {
                _resourceBindings[i].ssboView->release();
            }

            if (_resourceBindings[i].dabo != nullptr) {
                _resourceBindings[i].dabo->release();
            }

            _resourceBindings[i] = {};
            _resourceBindings[i].f = f;
            _resourceBindings[i].i = i;

            return true;
        }

        return false;
    }

    bool bindTile(const char* name, vkom::ITextureView* view) override {
        vkom::ITexture* texture = view->parent<vkom::ITexture>();
        if (texture == nullptr) {
            return false;
        }

        vkom::IStorageTexture* storageTexture = texture->queryInterface<vkom::IStorageTexture>();
        if (storageTexture == nullptr) {
            return false;
        }

        for (uint32_t i = 0; i < _tileInfos.size(); i += 1) {
            if (std::strcmp(_tileInfos[i].name, name) != 0) {
                continue;
            }

            if (_tileBindings[i] != nullptr) {
                _tileBindings[i]->release();
            }

            _tileBindings[i] = view;
            view->retain();

            return true;
        }

        return false;
    }

    bool bindTileBindingSet(uint32_t binding, uint32_t set, vkom::ITextureView* view) override {
        vkom::ITexture* texture = view->parent<vkom::ITexture>();
        if (texture == nullptr) {
            return false;
        }

        vkom::IStorageTexture* storageTexture = texture->queryInterface<vkom::IStorageTexture>();
        if (storageTexture == nullptr) {
            return false;
        }

        for (uint32_t i = 0; i < _tileInfos.size(); i += 1) {
            if (_tileInfos[i].binding != binding || _tileInfos[i].set != set) {
                continue;
            }

            if (_tileBindings[i] != nullptr) {
                _tileBindings[i]->release();
            }

            _tileBindings[i] = view;
            view->retain();

            return true;
        }

        return false;
    }

    bool execute(vkom::ICommandEncoder* encoder, vkom::IFence* executionFinishedFence) override {
        GeneralPassDispatchData& currentDispatch = _dispatchDatas[_dispatchID % DispatchesInFlight];
        if (!_isDescriptorless) {
            if (currentDispatch.fence != nullptr && !currentDispatch.fence->status()) {
                if (!blocking) {
                    return false;
                }

                assert(currentDispatch.fence->wait() == vkom::Result::Success);
                currentDispatch.fence->release();
            }

            currentDispatch.fence = executionFinishedFence;
            currentDispatch.fence->retain();
        }

        vkom::ComputePassDescriptor passDescriptor = {};
        vkom::IComputePass* pass = encoder->beginComputePass(&passDescriptor);

        pass->bindPipeline(_pipelineData.pipeline);
        for (GeneralPassDispatchDescriptorSetData const& set : currentDispatch.sets) {
            pass->bindDescriptorSet(_pipelineData.layout, set.setID, set.set, 0, nullptr);
        }

        size_t pushConstantDescriptorCount = 0;
        for (PassSlotResourceInfo const& info : _resourceInfos) {
            if (info.addressing.type != PassResourceAddressingType::DeviceAddress) {
                continue;
            }

            pushConstantDescriptorCount = std::max(pushConstantDescriptorCount, info.addressing.bindingOrSlot);
        }

        std::vector<PushConstantAudioBufferDescriptor> pushConstants(pushConstantDescriptorCount, {});
        for (size_t i = 0; i < _resourceInfos.size(); i += 1) {
            PassSlotResourceInfo const& info = _resourceInfos[i];
            GeneralPassResourceBindingData const& binding = _resourceBindings[i];

            if (info.addressing.type != PassResourceAddressingType::DeviceAddress) {
                continue;
            }

            if (binding.dabo != nullptr) {
                vkom::BufferInfo daboInfo = {};
                binding.dabo->getInfo(&daboInfo);

                pushConstants[info.addressing.bindingOrSlot].address = binding.dabo->deviceAddress();
                pushConstants[info.addressing.bindingOrSlot].count = daboInfo.size / sizeof(float);
            }
        }

        pass->pushConstants(_pipelineData.layout, vkom::ShaderStageFlags::Compute, 0, pushConstants.size() * sizeof(PushConstantAudioBufferDescriptor), pushConstants.data());

        pass->end();

        for (GeneralPassResourceBindingData const& binding : _resourceBindings) {
            if (binding.buffer != nullptr) {
                binding.buffer->release();
            }
        }

        for (vkom::ITextureView* tile : _tileBindings) {
            if (tile != nullptr) {
                tile->release();
            }
        }

        _resourceBindings.assign(_resourceBindings.size(), {});
        _tileBindings.assign(_tileBindings.size(), nullptr);

        _dispatchID += 1;
    }
};

vkom::SurfaceWSIInfo surfaceWSIInfoFromSDLWindow(SDL_Window* window) {
    vkom::SurfaceWSIInfo info = {};

    #ifdef VKOM_PLATFORM_FAMILY_NT
    info.type = vkom::SurfaceWSIType::Win32;
    info.windowHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    info.displayHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr));
    #elif defined(VKOM_PLATFORM_FAMILY_APPLE)
    SDL_MetalView view = SDL_Metal_CreateView(window);

    info.type = vkom::SurfaceWSIType::Metal;
    info.windowHandle = reinterpret_cast<uint64_t>(SDL_Metal_GetLayer(view));
    #else
    info.type = vkom::SurfaceWSIType::Xlib;
    info.windowHandle = reinterpret_cast<uint64_t>(SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, nullptr));
    info.displayHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));

    if (info.windowHandle == 0) {
        info.type = vkom::SurfaceWSIType::Wayland;
        info.windowHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));
        info.displayHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
    }
    #endif

    return info;
}

int main(int argc, char** argv) {
    klatr::gpu::Context gpuContext = klatr::gpu::Context(true);
    gpuContext.instance->setLogCallback([](vkom::IInstance* instance, void* userData, vkom::DebugMessageSeverityFlags severity, vkom::DebugMessageTypeFlags types, const char* message) {
        std::printf("[vkom]: %s\n", message);
    }, nullptr);

    assert(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* mainWindow = SDL_CreateWindow("klatr | main", 1200, 800, 0);
    assert(mainWindow != nullptr);

    vkom::IWSIInstance* wsiInstance = gpuContext.instance->queryInterface<vkom::IWSIInstance>();
    assert(wsiInstance != nullptr);

    vkom::IWSIDevice* wsiDevice = gpuContext.device->queryInterface<vkom::IWSIDevice>();
    assert(wsiDevice != nullptr);

    /* TODO: other platforms */
    vkom::SurfaceWSIInfo mainSurfaceInfo = surfaceWSIInfoFromSDLWindow(mainWindow);

    vkom::ISurface* mainSurface;
    assert(wsiInstance->createSurface(&mainSurfaceInfo, &mainSurface) == vkom::Result::Success);

    vkom::SurfaceCapabilities mainSurfaceCapabilities = {};
    gpuContext.adapter->querySurfaceCapabilities(mainSurface, &mainSurfaceCapabilities);

    vkom::SwapchainInfo mainSwapchainInfo = {};
    mainSwapchainInfo.backbufferCount = std::min(std::max(3u, mainSurfaceCapabilities.minBackbufferCount), mainSurfaceCapabilities.maxBackbufferCount);
    mainSwapchainInfo.backbufferInfo.samplesPerTexel = 1;
    mainSwapchainInfo.backbufferInfo.usage = vkom::TextureUsageFlags::TransferDestination | vkom::TextureUsageFlags::TransferSource;
    mainSwapchainInfo.backbufferInfo.dimensions.extent = mainSurfaceCapabilities.currentExtent;
    mainSwapchainInfo.backbufferInfo.dimensions.subresource.layers = 1;
    mainSwapchainInfo.backbufferInfo.dimensions.subresource.mips = 1;
    mainSwapchainInfo.backbufferInfo.location = vkom::MemoryLocationFlags::GPU;
    mainSwapchainInfo.preTransform = vkom::SurfaceTransformFlags::Identity;
    mainSwapchainInfo.compositeAlpha = vkom::CompositeAlphaFlags::Opaque;
    mainSwapchainInfo.surfaceFormatBits = gpuContext.adapter->querySurfaceFormatBits(mainSurface, vkom::Format::RGBA8UnsignedNormSRGB, vkom::ColorSpaceFlags::All) | gpuContext.adapter->querySurfaceFormatBits(mainSurface, vkom::Format::RGBA8UnsignedNorm, vkom::ColorSpaceFlags::All) | gpuContext.adapter->querySurfaceFormatBits(mainSurface, vkom::Format::BGRA8UnsignedNormSRGB, vkom::ColorSpaceFlags::All) | gpuContext.adapter->querySurfaceFormatBits(mainSurface, vkom::Format::BGRA8UnsignedNorm, vkom::ColorSpaceFlags::All);
    mainSwapchainInfo.presentModeFlags = gpuContext.adapter->querySurfacePresentModes(mainSurface);

    vkom::ISwapchain* mainSwapchain;
    assert(wsiDevice->createSwapchain(mainSurface, &mainSwapchainInfo, &mainSwapchain) == vkom::Result::Success);

    vkom::IHeap* cpuEndpointBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::StorageBuffer | vkom::BufferUsageFlags::ShaderDeviceAddress, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::CPU, &cpuEndpointBufferHeap) == vkom::Result::Success);

    vkom::IHeap* gpuTransientBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::StorageBuffer | vkom::BufferUsageFlags::ShaderDeviceAddress, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::GPU, &gpuTransientBufferHeap) == vkom::Result::Success);

    vkom::IHeap* cpuUniformBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::UniformBuffer, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::CPU, &cpuUniformBufferHeap) == vkom::Result::Success);

    vkom::IHeap* gpuModuleTileTextureHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::None, vkom::TextureUsageFlags::TransferDestination | vkom::TextureUsageFlags::TransferSource | vkom::TextureUsageFlags::Storage, vkom::MemoryLocationFlags::GPU, &gpuModuleTileTextureHeap) == vkom::Result::Success);

    vkom::DescriptorBindingInfo moduleCommonUniformsDescriptorSetLayoutBindingInfos[1] = {};
    moduleCommonUniformsDescriptorSetLayoutBindingInfos[0].binding = 0;
    moduleCommonUniformsDescriptorSetLayoutBindingInfos[0].flags = vkom::DescriptorFlags::UniformBuffer;
    moduleCommonUniformsDescriptorSetLayoutBindingInfos[0].count = 1;
    moduleCommonUniformsDescriptorSetLayoutBindingInfos[0].stages = vkom::ShaderStageFlags::Compute;

    vkom::DescriptorSetLayoutInfo moduleCommonUniformsDescriptorSetLayoutInfo = {};
    moduleCommonUniformsDescriptorSetLayoutInfo.bindingCount = 1;
    moduleCommonUniformsDescriptorSetLayoutInfo.bindings = &moduleCommonUniformsDescriptorSetLayoutBindingInfos[0];

    vkom::IDescriptorSetLayout* moduleCommonUniformsDescriptorSetLayout;
    assert(gpuContext.device->createDescriptorSetLayout(&moduleCommonUniformsDescriptorSetLayoutInfo, &moduleCommonUniformsDescriptorSetLayout) == vkom::Result::Success);

    vkom::DescriptorBindingInfo moduleTileDescriptorSetLayoutBindingInfos[1] = {};
    moduleTileDescriptorSetLayoutBindingInfos[0].binding = 0;
    moduleTileDescriptorSetLayoutBindingInfos[0].flags = vkom::DescriptorFlags::StorageTexture;
    moduleTileDescriptorSetLayoutBindingInfos[0].count = 1;
    moduleTileDescriptorSetLayoutBindingInfos[0].stages = vkom::ShaderStageFlags::Compute;

    vkom::DescriptorSetLayoutInfo moduleTileDescriptorSetLayoutInfo = {};
    moduleTileDescriptorSetLayoutInfo.bindingCount = 1;
    moduleTileDescriptorSetLayoutInfo.bindings = &moduleTileDescriptorSetLayoutBindingInfos[0];

    vkom::IDescriptorSetLayout* moduleTileDescriptorSetLayout;
    assert(gpuContext.device->createDescriptorSetLayout(&moduleTileDescriptorSetLayoutInfo, &moduleTileDescriptorSetLayout) == vkom::Result::Success);

    /* TODO: more accurate count of descriptor types */
    vkom::DescriptorPoolDescriptorInfo moduleDescriptorPoolDescriptorInfos[2] = {};
    moduleDescriptorPoolDescriptorInfos[0].flags = vkom::DescriptorFlags::UniformBuffer;
    moduleDescriptorPoolDescriptorInfos[0].count = 256;
    moduleDescriptorPoolDescriptorInfos[1].flags = vkom::DescriptorFlags::StorageTexture;
    moduleDescriptorPoolDescriptorInfos[1].count = 256;

    /* TODO: more accurate count of descriptor sets */
    vkom::DescriptorPoolInfo moduleDescriptorPoolInfo = {};
    moduleDescriptorPoolInfo.maxDescriptorSets = 256;
    moduleDescriptorPoolInfo.descriptorCount = 2;
    moduleDescriptorPoolInfo.descriptors = &moduleDescriptorPoolDescriptorInfos[0];

    vkom::IDescriptorPool* moduleDescriptorPool;
    assert(gpuContext.device->createDescriptorPool(&moduleDescriptorPoolInfo, &moduleDescriptorPool) == vkom::Result::Success);

    vkom::IDescriptorSet* moduleCommonUniformsDescriptorSet;
    assert(moduleDescriptorPool->allocateDescriptorSets(moduleCommonUniformsDescriptorSetLayout, 1, &moduleCommonUniformsDescriptorSet) == vkom::Result::Success);

    vkom::IDescriptorSet* moduleTileDescriptorSet;
    assert(moduleDescriptorPool->allocateDescriptorSets(moduleTileDescriptorSetLayout, 1, &moduleTileDescriptorSet) == vkom::Result::Success);

    vkom::PushConstantRange modulePipelineLayoutPushConstantRanges[1] = {};
    modulePipelineLayoutPushConstantRanges[0].offset = 0;
    modulePipelineLayoutPushConstantRanges[0].size = sizeof(PushConstantAudioBufferDescriptor);
    modulePipelineLayoutPushConstantRanges[0].stages = vkom::ShaderStageFlags::Compute;

    vkom::IDescriptorSetLayout* modulePipelineLayoutDescriptorSetLayouts[2] = {};
    modulePipelineLayoutDescriptorSetLayouts[0] = moduleCommonUniformsDescriptorSetLayout;
    modulePipelineLayoutDescriptorSetLayouts[1] = moduleTileDescriptorSetLayout;

    vkom::PipelineLayoutInfo modulePipelineLayoutInfo = {};
    modulePipelineLayoutInfo.descriptorSetLayoutCount = 2;
    modulePipelineLayoutInfo.descriptorSetLayouts = &modulePipelineLayoutDescriptorSetLayouts[0];
    modulePipelineLayoutInfo.pushConstantRangeCount = 1;
    modulePipelineLayoutInfo.pushConstantRanges = &modulePipelineLayoutPushConstantRanges[0];

    vkom::IPipelineLayout* modulePipelineLayout;
    assert(gpuContext.device->createPipelineLayout(&modulePipelineLayoutInfo, &modulePipelineLayout) == vkom::Result::Success);

    ModuleInfo defaultModule = {};
    vkom::IComputePipeline* defaultModulePipeline = loadModuleAndPipeline(defaultModule, gpuContext.device, modulePipelineLayout, "module.hlsl.spv");
    assert(defaultModulePipeline != nullptr);

    vkom::TextureInfo gpuModuleTileTextureInfo = {};
    gpuModuleTileTextureInfo.format = vkom::Format::RGBA8UnsignedNorm;
    gpuModuleTileTextureInfo.samplesPerTexel = 1;
    gpuModuleTileTextureInfo.usage = vkom::TextureUsageFlags::TransferDestination | vkom::TextureUsageFlags::TransferSource | vkom::TextureUsageFlags::Storage;
    gpuModuleTileTextureInfo.dimensions.extent.width = 512;
    gpuModuleTileTextureInfo.dimensions.extent.height = 512;
    gpuModuleTileTextureInfo.dimensions.subresource.layers = 1;
    gpuModuleTileTextureInfo.dimensions.subresource.mips = 1;
    gpuModuleTileTextureInfo.location = vkom::MemoryLocationFlags::GPU;

    vkom::ITexture* gpuModuleTileTexture;
    assert(gpuModuleTileTextureHeap->createTexture(&gpuModuleTileTextureInfo, &gpuModuleTileTexture) == vkom::Result::Success);

    vkom::TextureViewInfo gpuModuleTileTextureViewInfo = {};
    gpuModuleTileTextureViewInfo.format = gpuModuleTileTextureInfo.format;
    gpuModuleTileTextureViewInfo.type = vkom::TextureViewType::D2;
    gpuModuleTileTextureViewInfo.aspectFlags = vkom::TextureAspectFlags::Color;
    gpuModuleTileTextureViewInfo.subresourceDimensions.layers = 1;
    gpuModuleTileTextureViewInfo.subresourceDimensions.mips = 1;
    gpuModuleTileTextureViewInfo.subresourcePosition.layer = 0;
    gpuModuleTileTextureViewInfo.subresourcePosition.mip = 0;

    vkom::ITextureView* gpuModuleTileTextureView;
    assert(gpuModuleTileTexture->createView(&gpuModuleTileTextureViewInfo, &gpuModuleTileTextureView) == vkom::Result::Success);

    vkom::IFence* audioComputeBatchFinishedFence;
    assert(gpuContext.device->acquireFence(false, &audioComputeBatchFinishedFence) == vkom::Result::Success);

    vkom::IFence* presentBatchFinishedFence;
    assert(gpuContext.device->acquireFence(false, &presentBatchFinishedFence) == vkom::Result::Success);

    vkom::IPresentFence* mainSwapchainPresentationFinishedFence = nullptr;

    vkom::ISemaphore* mainBackbufferAcquisitionSemaphore;
    assert(gpuContext.device->acquireSemaphore(false, &mainBackbufferAcquisitionSemaphore) == vkom::Result::Success);

    vkom::ISemaphore* mainBackbufferRenderFinishedSemaphore;
    assert(gpuContext.device->acquireSemaphore(false, &mainBackbufferRenderFinishedSemaphore) == vkom::Result::Success);

    vkom::BufferInfo cpuEndpointBufferInfo = {};
    cpuEndpointBufferInfo.size = sizeof(float) * ((INTERNAL_AUDIO_BUFFER_FRAME_COUNT + 512) / 1024) * 1024;
    cpuEndpointBufferInfo.usage = vkom::BufferUsageFlags::TransferDestination | vkom::BufferUsageFlags::StorageBuffer | vkom::BufferUsageFlags::ShaderDeviceAddress;
    cpuEndpointBufferInfo.location = vkom::MemoryLocationFlags::CPU;

    vkom::IBuffer* cpuEndpointBuffer;
    assert(cpuEndpointBufferHeap->createBuffer(&cpuEndpointBufferInfo, &cpuEndpointBuffer) == vkom::Result::Success);

    vkom::BufferViewInfo cpuEndpointBufferViewInfo = {};
    cpuEndpointBufferViewInfo.offset = 0;
    cpuEndpointBufferViewInfo.range = cpuEndpointBufferInfo.size;

    vkom::IBufferView* cpuEndpointBufferView;
    assert(cpuEndpointBuffer->createView(&cpuEndpointBufferViewInfo, &cpuEndpointBufferView) == vkom::Result::Success);

    vkom::IStorageBuffer* cpuEndpointBufferSSBO = cpuEndpointBuffer->queryInterface<vkom::IStorageBuffer>();
    assert(cpuEndpointBufferSSBO != nullptr);

    vkom::IDeviceAddressBuffer* cpuEndpointBufferDA = cpuEndpointBuffer->queryInterface<vkom::IDeviceAddressBuffer>();
    assert(cpuEndpointBufferDA != nullptr);

    vkom::BufferInfo cpuUniformBufferInfo = {};
    cpuUniformBufferInfo.size = sizeof(Uniforms);
    cpuUniformBufferInfo.usage = vkom::BufferUsageFlags::UniformBuffer;
    cpuUniformBufferInfo.location = vkom::MemoryLocationFlags::CPU;

    vkom::IBuffer* cpuUniformBuffer;
    assert(cpuUniformBufferHeap->createBuffer(&cpuUniformBufferInfo, &cpuUniformBuffer) == vkom::Result::Success);

    vkom::BufferViewInfo cpuUniformBufferViewInfo = {};
    cpuUniformBufferViewInfo.offset = 0;
    cpuUniformBufferViewInfo.range = cpuUniformBufferInfo.size;

    vkom::IBufferView* cpuUniformBufferView;
    assert(cpuUniformBuffer->createView(&cpuUniformBufferViewInfo, &cpuUniformBufferView) == vkom::Result::Success);

    vkom::IUniformBuffer* cpuUniformBufferUBO = cpuUniformBuffer->queryInterface<vkom::IUniformBuffer>();
    assert(cpuUniformBufferUBO != nullptr);

    PlaybackBuffer playbackBuffer = {};

    ma_log maContextLogger = {};
    maContextLogger.callbacks[0].onLog = [](void* user, ma_uint32 level, const char* message) {
        std::printf("[miniaudio(%u)]: %s", level, message);
    };

    maContextLogger.callbackCount = 1;

    ma_context_config maContextConfig = {};
    maContextConfig.pLog = &maContextLogger;
    maContextConfig.threadPriority = ma_thread_priority_default;

    ma_context maContext = {};
    assert(ma_context_init(nullptr, 0, &maContextConfig, &maContext) == MA_SUCCESS);

    ma_device_config maPlaybackDeviceConfig = {};
    maPlaybackDeviceConfig.deviceType = ma_device_type_playback;
    maPlaybackDeviceConfig.sampleRate = 44100;
    maPlaybackDeviceConfig.dataCallback = [](ma_device* device, void* output, void const* input, ma_uint32 frameCount) {
        assert(device->playback.format == ma_format_f32);

        PlaybackBuffer* playbackBuffer = reinterpret_cast<PlaybackBuffer*>(device->pUserData);
        if (playbackBuffer == nullptr) {
            return;
        }

        size_t count = frameCount * device->playback.channels;
        //ma_log_postf(device->pContext->pLog, MA_LOG_LEVEL_INFO, "%zu samples requested, %zu samples received\n", count, playbackBuffer->consume(count, reinterpret_cast<float*>(output), false));
    };

    maPlaybackDeviceConfig.pUserData = &playbackBuffer;
    maPlaybackDeviceConfig.playback.format = ma_format_f32;
    maPlaybackDeviceConfig.playback.channels = 1;

    ma_device maPlaybackDevice = {};
    assert(ma_device_init(&maContext, &maPlaybackDeviceConfig, &maPlaybackDevice) == MA_SUCCESS);
    assert(ma_device_start(&maPlaybackDevice) == MA_SUCCESS);

    uint64_t globalID = 0;
    uint32_t previousPlayedUntil = 0;

    bool presentInProgress = false;
    bool audioComputeBatchInProgress = false;
    vkom::ICommandEncoder* audioComputeEncoder = nullptr;
    vkom::ICommandBatch* audioComputeBatch = nullptr;

    vkom::ICommandEncoder* presentEncoder = nullptr;
    vkom::ICommandBatch* presentBatch = nullptr;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_S) {
                        ma_encoder_config maEncoderConfig = ma_encoder_config_init(ma_encoding_format_wav, maPlaybackDeviceConfig.playback.format, maPlaybackDeviceConfig.playback.channels, maPlaybackDeviceConfig.sampleRate);

                        ma_encoder maEncoder = {};
                        assert(ma_encoder_init_file("out.wav", &maEncoderConfig, &maEncoder) == MA_SUCCESS);

                        assert(ma_encoder_write_pcm_frames(&maEncoder, playbackBuffer.data(), playbackBuffer.capacity() / maPlaybackDeviceConfig.playback.channels, nullptr) == MA_SUCCESS);
                        ma_encoder_uninit(&maEncoder);
                    }
                    break;
                default:
                    break;
            }
        }

        if (presentInProgress) {
            if (mainSwapchainPresentationFinishedFence != nullptr && mainSwapchainPresentationFinishedFence->status()) {
                mainSwapchainPresentationFinishedFence->release();
                mainSwapchainPresentationFinishedFence = nullptr;

                presentInProgress = false;
            }
        }

        vkom::IBackbuffer* mainBackbuffer = nullptr;
        if (!presentInProgress) {
            vkom::SemaphorePoint mainBackbufferAcquisitionSignal = {};
            mainBackbufferAcquisitionSignal.semaphore = mainBackbufferAcquisitionSemaphore;

            uint32_t mainBackbufferIndex;
            vkom::Result mainBackbufferAcquisitionResult = mainSwapchain->acquireNextIndex(&mainBackbufferAcquisitionSignal, nullptr, &mainBackbufferIndex);
            /* TODO: handle resize */
            assert(mainBackbufferAcquisitionResult == vkom::Result::Success);

            mainBackbuffer = mainSwapchain->enumerateBackbuffers(mainBackbufferIndex);
            assert(mainBackbuffer != nullptr);
        }

        if (audioComputeBatchFinishedFence->status()) {
            assert(audioComputeBatchFinishedFence->reset() == vkom::Result::Success);

            if (audioComputeBatch != nullptr) {
                audioComputeBatch->discard();
                audioComputeEncoder->release();

                audioComputeBatch = nullptr;
                audioComputeEncoder = nullptr;
            }

            audioComputeBatchInProgress = false;

            float* cpuEndpointBufferMapped = reinterpret_cast<float*>(cpuEndpointBuffer->map());
            assert(playbackBuffer.produce(INTERNAL_AUDIO_BUFFER_FRAME_COUNT * maPlaybackDevice.playback.channels, cpuEndpointBufferMapped, true));
            cpuEndpointBuffer->unmap();
        }

        if (!audioComputeBatchInProgress) {
            vkom::IResourceView* cpuUniformBufferResourceView = cpuUniformBufferView->queryInterface<vkom::IResourceView>();

            vkom::DescriptorWrite moduleCommonUniformsDescriptorSetWrite = {};
            moduleCommonUniformsDescriptorSetWrite.binding = 0;
            moduleCommonUniformsDescriptorSetWrite.element = 0;
            moduleCommonUniformsDescriptorSetWrite.count = 1;
            moduleCommonUniformsDescriptorSetWrite.views = &cpuUniformBufferResourceView;

            moduleCommonUniformsDescriptorSet->write(1, &moduleCommonUniformsDescriptorSetWrite);

            vkom::DescriptorTextureInfo gpuModuleTileDescriptorTextureInfo = {};
            gpuModuleTileDescriptorTextureInfo.layout = vkom::TextureLayout::General;

            vkom::IResourceView* gpuModuleTileTextureResourceView = gpuModuleTileTextureView->queryInterface<vkom::IResourceView>();

            vkom::DescriptorWrite moduleTileDescriptorSetWrite = {};
            moduleTileDescriptorSetWrite.binding = 0;
            moduleTileDescriptorSetWrite.element = 0;
            moduleTileDescriptorSetWrite.count = 1;
            moduleTileDescriptorSetWrite.textureInfos = &gpuModuleTileDescriptorTextureInfo;
            moduleTileDescriptorSetWrite.views = &gpuModuleTileTextureResourceView;

            moduleTileDescriptorSet->write(1, &moduleTileDescriptorSetWrite);

            assert(gpuContext.audioComputeQueue->acquireCommandEncoder(&audioComputeEncoder) == vkom::Result::Success);

            vkom::ITransferDestinationBuffer* cpuEndpointBufferTD = cpuEndpointBuffer->queryInterface<vkom::ITransferDestinationBuffer>();
            assert(cpuEndpointBufferTD != nullptr);

            vkom::BufferFill cpuEndpointBufferFill = {};
            cpuEndpointBufferFill.dstOffset = 0;
            cpuEndpointBufferFill.size = cpuEndpointBufferInfo.size;
            cpuEndpointBufferFill.word = 0;

            audioComputeEncoder->fillBuffer(cpuEndpointBufferTD, &cpuEndpointBufferFill);

            vkom::TextureTransition moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute = {};
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.general.srcStage = vkom::PipelineStageFlags::Transfer;
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.general.dstStage = vkom::PipelineStageFlags::ComputeShader;
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.transfer.oldFamily = gpuContext.presentQueue->family();
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.transfer.newFamily = gpuContext.audioComputeQueue->family();
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.oldLayout = vkom::TextureLayout::Undefined;
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.newLayout = vkom::TextureLayout::General;
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.aspectFlags = vkom::TextureAspectFlags::Color;
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.subresourcePosition.layer = 0;
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.subresourcePosition.mip = 0;
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.subresourceDimensions.layers = 1;
            moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.subresourceDimensions.mips = 1;

            audioComputeEncoder->transitionTexture(gpuModuleTileTexture, &moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute);

            vkom::ColorTextureClear moduleTileClear = {};
            moduleTileClear.layout = vkom::TextureLayout::General;
            moduleTileClear.color[0] = 0.02f;
            moduleTileClear.color[1] = 0.02f;
            moduleTileClear.color[2] = 0.02f;
            moduleTileClear.color[3] = 1.0f;
            moduleTileClear.subresourceOffset.layer = 0;
            moduleTileClear.subresourceOffset.mip = 0;
            moduleTileClear.subresourceRange.layers = 1;
            moduleTileClear.subresourceRange.mips = 1;

            audioComputeEncoder->clearColorTexture(gpuModuleTileTexture->queryInterface<vkom::ITransferDestinationTexture>(), &moduleTileClear);

            vkom::ComputePassDescriptor cpDescriptor = {};

            vkom::IComputePass* cp = audioComputeEncoder->beginComputePass(&cpDescriptor);
            assert(cp != nullptr);

            cp->bindPipeline(defaultModulePipeline);
            cp->bindDescriptorSet(modulePipelineLayout, 0, moduleCommonUniformsDescriptorSet, 0, nullptr);
            cp->bindDescriptorSet(modulePipelineLayout, 1, moduleTileDescriptorSet, 0, nullptr);

            Uniforms uniforms = {};
            uniforms.globalID = globalID;
            uniforms.globalTime = static_cast<float>(globalID) / static_cast<float>(maPlaybackDeviceConfig.sampleRate);
            uniforms.dispatchWidth = (INTERNAL_AUDIO_BUFFER_FRAME_COUNT + 1023) / 1024;
            uniforms.sampleRate = maPlaybackDeviceConfig.sampleRate;
            uniforms.inverseSampleRate = 1.0f / static_cast<float>(uniforms.sampleRate);

            globalID += INTERNAL_AUDIO_BUFFER_FRAME_COUNT;

            //std::printf("%u\n", globalID);

            void* mappedUniformBuffer = cpuUniformBuffer->map();
            std::memcpy(mappedUniformBuffer, &uniforms, sizeof(Uniforms));
            cpuUniformBuffer->unmap();

            PushConstants pushConstants = {};
            pushConstants.buffer.address = cpuEndpointBufferDA->deviceAddress();
            pushConstants.buffer.count = INTERNAL_AUDIO_BUFFER_FRAME_COUNT;
            pushConstants.buffer.bounds[0] = -1.0f;
            pushConstants.buffer.bounds[1] = 1.0f;

            cp->pushConstants(modulePipelineLayout, vkom::ShaderStageFlags::Compute, 0, sizeof(pushConstants), &pushConstants);
            cp->dispatch(uniforms.dispatchWidth, 1, 1);

            cp->end();
            cp = nullptr;

            vkom::TextureTransition moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent = {};
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.general.srcStage = vkom::PipelineStageFlags::ComputeShader;
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.general.dstStage = vkom::PipelineStageFlags::Transfer;
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.transfer.oldFamily = gpuContext.audioComputeQueue->family();
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.transfer.newFamily = gpuContext.presentQueue->family();
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.oldLayout = vkom::TextureLayout::General;
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.newLayout = vkom::TextureLayout::TransferSource;
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.aspectFlags = vkom::TextureAspectFlags::Color;
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.subresourcePosition.layer = 0;
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.subresourcePosition.mip = 0;
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.subresourceDimensions.layers = 1;
            moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.subresourceDimensions.mips = 1;

            audioComputeEncoder->transitionTexture(gpuModuleTileTexture, &moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent);

            assert(audioComputeEncoder->batch(&audioComputeBatch) == vkom::Result::Success);

            vkom::CommandBatchSubmitInfo submitInfo = {};
            submitInfo.signalFence = audioComputeBatchFinishedFence;

            assert(audioComputeBatch->submit(&submitInfo) == vkom::Result::Success);
        }

        if (presentBatchFinishedFence->status()) {
            presentBatchFinishedFence->reset();

            presentBatch->discard();
            presentEncoder->release();

            presentBatch = nullptr;
            presentEncoder = nullptr;
        }

        if (mainBackbuffer != nullptr && presentEncoder == nullptr && mainSwapchainPresentationFinishedFence == nullptr) {
            assert(gpuContext.presentQueue->acquireCommandEncoder(&presentEncoder) == vkom::Result::Success);

            vkom::TextureTransition mainBackbufferTransitionToTransferDestination = {};
            mainBackbufferTransitionToTransferDestination.general.srcStage = vkom::PipelineStageFlags::TopOfPipe;
            mainBackbufferTransitionToTransferDestination.general.dstStage = vkom::PipelineStageFlags::TopOfPipe;
            mainBackbufferTransitionToTransferDestination.transfer.oldFamily = gpuContext.presentQueue->family();
            mainBackbufferTransitionToTransferDestination.transfer.newFamily = gpuContext.presentQueue->family();
            mainBackbufferTransitionToTransferDestination.oldLayout = vkom::TextureLayout::Undefined;
            mainBackbufferTransitionToTransferDestination.newLayout = vkom::TextureLayout::TransferDestination;
            mainBackbufferTransitionToTransferDestination.aspectFlags = vkom::TextureAspectFlags::Color;
            mainBackbufferTransitionToTransferDestination.subresourcePosition.layer = 0;
            mainBackbufferTransitionToTransferDestination.subresourcePosition.mip = 0;
            mainBackbufferTransitionToTransferDestination.subresourceDimensions.layers = 1;
            mainBackbufferTransitionToTransferDestination.subresourceDimensions.mips = 1;

            presentEncoder->transitionTexture(mainBackbuffer, &mainBackbufferTransitionToTransferDestination);

            vkom::ColorTextureClear mainBackbufferClear = {};
            mainBackbufferClear.layout = vkom::TextureLayout::TransferDestination;
            mainBackbufferClear.color[0] = 1.0f;
            mainBackbufferClear.color[1] = 0.0f;
            mainBackbufferClear.color[2] = 1.0f;
            mainBackbufferClear.color[3] = 1.0f;
            mainBackbufferClear.subresourceOffset.layer = 0;
            mainBackbufferClear.subresourceOffset.mip = 0;
            mainBackbufferClear.subresourceRange.layers = 1;
            mainBackbufferClear.subresourceRange.mips = 1;

            presentEncoder->clearColorTexture(mainBackbuffer->queryInterface<vkom::ITransferDestinationTexture>(), &mainBackbufferClear);

            vkom::TextureTransition moduleTileTransitionTransferSourceAndAcquireOwnershipFromComputeToPresent = {};
            mainBackbufferTransitionToTransferDestination.general.srcStage = vkom::PipelineStageFlags::ComputeShader;
            mainBackbufferTransitionToTransferDestination.general.dstStage = vkom::PipelineStageFlags::Transfer;
            mainBackbufferTransitionToTransferDestination.transfer.oldFamily = gpuContext.audioComputeQueue->family();
            mainBackbufferTransitionToTransferDestination.transfer.newFamily = gpuContext.presentQueue->family();
            mainBackbufferTransitionToTransferDestination.oldLayout = vkom::TextureLayout::Undefined;
            mainBackbufferTransitionToTransferDestination.newLayout = vkom::TextureLayout::TransferSource;
            mainBackbufferTransitionToTransferDestination.aspectFlags = vkom::TextureAspectFlags::Color;
            mainBackbufferTransitionToTransferDestination.subresourcePosition.layer = 0;
            mainBackbufferTransitionToTransferDestination.subresourcePosition.mip = 0;
            mainBackbufferTransitionToTransferDestination.subresourceDimensions.layers = 1;
            mainBackbufferTransitionToTransferDestination.subresourceDimensions.mips = 1;

            presentEncoder->transitionTexture(gpuModuleTileTexture, &mainBackbufferTransitionToTransferDestination);

            vkom::TextureBlit blitModuleTileToMainBackbuffer = {};
            blitModuleTileToMainBackbuffer.srcLayout = vkom::TextureLayout::TransferSource;
            blitModuleTileToMainBackbuffer.srcPositions[0].xyz.x = 0;
            blitModuleTileToMainBackbuffer.srcPositions[0].xyz.y = 0;
            blitModuleTileToMainBackbuffer.srcPositions[0].xyz.z = 0;
            blitModuleTileToMainBackbuffer.srcPositions[0].subresource.layer = 0;
            blitModuleTileToMainBackbuffer.srcPositions[0].subresource.mip = 0;
            blitModuleTileToMainBackbuffer.srcPositions[1].xyz.x = gpuModuleTileTextureInfo.dimensions.extent.width;
            blitModuleTileToMainBackbuffer.srcPositions[1].xyz.y = gpuModuleTileTextureInfo.dimensions.extent.height;
            blitModuleTileToMainBackbuffer.srcPositions[1].xyz.z = std::max(1u, gpuModuleTileTextureInfo.dimensions.extent.depth);
            blitModuleTileToMainBackbuffer.srcPositions[1].subresource.layer = 0;
            blitModuleTileToMainBackbuffer.srcPositions[1].subresource.mip = 1;
            blitModuleTileToMainBackbuffer.dstLayout = vkom::TextureLayout::TransferDestination;
            blitModuleTileToMainBackbuffer.dstPositions[0].xyz.x = 0;
            blitModuleTileToMainBackbuffer.dstPositions[0].xyz.y = 0;
            blitModuleTileToMainBackbuffer.dstPositions[0].xyz.z = 0;
            blitModuleTileToMainBackbuffer.dstPositions[0].subresource.layer = 0;
            blitModuleTileToMainBackbuffer.dstPositions[0].subresource.mip = 0;
            blitModuleTileToMainBackbuffer.dstPositions[1].xyz.x = mainSurfaceCapabilities.currentExtent.width;
            blitModuleTileToMainBackbuffer.dstPositions[1].xyz.y = mainSurfaceCapabilities.currentExtent.height;
            blitModuleTileToMainBackbuffer.dstPositions[1].xyz.z = std::max(1u, mainSurfaceCapabilities.currentExtent.depth);
            blitModuleTileToMainBackbuffer.dstPositions[1].subresource.layer = 0;
            blitModuleTileToMainBackbuffer.dstPositions[1].subresource.mip = 1;
            blitModuleTileToMainBackbuffer.aspectFlags = vkom::TextureAspectFlags::Color;
            blitModuleTileToMainBackbuffer.filter = vkom::TexelFilter::Nearest;

            presentEncoder->blitTexture(mainBackbuffer->queryInterface<vkom::ITransferDestinationTexture>(), gpuModuleTileTexture->queryInterface<vkom::ITransferSourceTexture>(), &blitModuleTileToMainBackbuffer);

            vkom::TextureTransition moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute = {};
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.general.srcStage = vkom::PipelineStageFlags::Transfer;
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.general.dstStage = vkom::PipelineStageFlags::ComputeShader;
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.transfer.oldFamily = gpuContext.presentQueue->family();
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.transfer.newFamily = gpuContext.audioComputeQueue->family();
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.oldLayout = vkom::TextureLayout::TransferSource;
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.newLayout = vkom::TextureLayout::General;
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.aspectFlags = vkom::TextureAspectFlags::Color;
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.subresourcePosition.layer = 0;
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.subresourcePosition.mip = 0;
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.subresourceDimensions.layers = 1;
            moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.subresourceDimensions.mips = 1;

            presentEncoder->transitionTexture(gpuModuleTileTexture, &moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute);

            vkom::TextureTransition mainBackbufferTransitionFromTransferDestinationToPresentSource = {};
            mainBackbufferTransitionFromTransferDestinationToPresentSource.general.srcStage = vkom::PipelineStageFlags::Transfer;
            mainBackbufferTransitionFromTransferDestinationToPresentSource.general.dstStage = vkom::PipelineStageFlags::TopOfPipe;
            mainBackbufferTransitionFromTransferDestinationToPresentSource.transfer.oldFamily = gpuContext.presentQueue->family();
            mainBackbufferTransitionFromTransferDestinationToPresentSource.transfer.newFamily = gpuContext.presentQueue->family();
            mainBackbufferTransitionFromTransferDestinationToPresentSource.oldLayout = vkom::TextureLayout::TransferDestination;
            mainBackbufferTransitionFromTransferDestinationToPresentSource.newLayout = vkom::TextureLayout::PresentSource;
            mainBackbufferTransitionFromTransferDestinationToPresentSource.aspectFlags = vkom::TextureAspectFlags::Color;
            mainBackbufferTransitionFromTransferDestinationToPresentSource.subresourcePosition.layer = 0;
            mainBackbufferTransitionFromTransferDestinationToPresentSource.subresourcePosition.mip = 0;
            mainBackbufferTransitionFromTransferDestinationToPresentSource.subresourceDimensions.layers = 1;
            mainBackbufferTransitionFromTransferDestinationToPresentSource.subresourceDimensions.mips = 1;

            presentEncoder->transitionTexture(mainBackbuffer, &mainBackbufferTransitionFromTransferDestinationToPresentSource);

            assert(presentEncoder->batch(&presentBatch) == vkom::Result::Success);

            vkom::CommandBatchSubmitWaitInfo presentBatchSubmitWaits[1] = {};
            presentBatchSubmitWaits[0].point.semaphore = mainBackbufferAcquisitionSemaphore;
            presentBatchSubmitWaits[0].stageFlags = vkom::PipelineStageFlags::TopOfPipe;

            vkom::CommandBatchSubmitSignalInfo presentBatchSubmitSignals[1] = {};
            presentBatchSubmitSignals[0].point.semaphore = mainBackbufferRenderFinishedSemaphore;

            vkom::CommandBatchSubmitInfo presentBatchSubmitInfo = {};
            presentBatchSubmitInfo.waitCount = 1;
            presentBatchSubmitInfo.waits = &presentBatchSubmitWaits[0];
            presentBatchSubmitInfo.signalCount = 1;
            presentBatchSubmitInfo.signals = &presentBatchSubmitSignals[0];
            presentBatchSubmitInfo.signalFence = presentBatchFinishedFence;

            assert(presentBatch->submit(&presentBatchSubmitInfo) == vkom::Result::Success);

            vkom::PresentInfo presentInfo = {};
            presentInfo.waitCount = 1;
            presentInfo.waits = &presentBatchSubmitSignals[0].point;

            vkom::Result mainSwapchainPresentationResult = mainSwapchain->present(gpuContext.presentQueue, mainBackbuffer, &presentInfo, &mainSwapchainPresentationFinishedFence);
            /* TODO: */
            assert(mainSwapchainPresentationResult == vkom::Result::Success);

            presentInProgress = true;
            mainBackbuffer->release();
        }

        ModuleInfo newDefaultModule = {};
        vkom::IComputePipeline* newDefaultModulePipeline = loadModuleAndPipeline(newDefaultModule, gpuContext.device, modulePipelineLayout, "module.hlsl.spv", defaultModule.lastModified);
        if (newDefaultModulePipeline != nullptr) {
            gpuContext.device->waitIdle();
            defaultModulePipeline->release();
            defaultModule.shader->release();

            defaultModulePipeline = newDefaultModulePipeline;
            defaultModule = newDefaultModule;
        }
    }

    SDL_DestroyWindow(mainWindow);
    SDL_Quit();

    return 0;
}

/*
int main(int argc, char** argv) {
    klatr::audio::IInstance* instance = klatr::audio::createInstance(klatr::audio::InstanceBackendFlags::Any);
    assert(instance != nullptr);

    klatr::audio::IInputAdapter* inputAdapter = instance->defaultAdapter<klatr::audio::IInputAdapter>(klatr::audio::DeviceFlowFlags::Input);
    assert(inputAdapter != nullptr);

    klatr::audio::AdapterInfo inputAdapterInfo = {};
    inputAdapter->getInfo(&inputAdapterInfo);

    klatr::audio::IOutputAdapter* outputAdapter = instance->defaultAdapter<klatr::audio::IOutputAdapter>(klatr::audio::DeviceFlowFlags::Output);
    assert(outputAdapter != nullptr);

    klatr::audio::AdapterInfo outputAdapterInfo = {};
    outputAdapter->getInfo(&outputAdapterInfo);

    klatr::audio::DeviceInfo deviceInfo = {};
    deviceInfo.flow = klatr::audio::DeviceFlowFlags::Input;
    deviceInfo.format = klatr::audio::FormatFlags::Float32;
    deviceInfo.sampleRate = inputAdapterInfo.highestTypicalSampleRate;
    deviceInfo.sampleCount = inputAdapterInfo.highestTypicalSampleRate / 100 * 2;

    klatr::audio::IInputDevice* inputDevice = inputAdapter->createDevice(&deviceInfo)->queryInterface<klatr::audio::IInputDevice>();
    assert(inputDevice != nullptr);

    deviceInfo = {};
    deviceInfo.flow = klatr::audio::DeviceFlowFlags::Output;
    deviceInfo.format = klatr::audio::FormatFlags::Float32;
    deviceInfo.sampleRate = outputAdapterInfo.highestTypicalSampleRate;
    deviceInfo.sampleCount = outputAdapterInfo.highestTypicalSampleRate / 100 * 2;

    klatr::audio::IOutputDevice* outputDevice = outputAdapter->createDevice(&deviceInfo)->queryInterface<klatr::audio::IOutputDevice>();
    assert(outputDevice != nullptr);

    outputDevice->start(klatr::audio::DeviceFlowFlags::Output);
    inputDevice->start(klatr::audio::DeviceFlowFlags::Input);

    while (true) {
        klatr::audio::IOutputBuffer* outputBuffer = outputDevice->acquireOutputBuffer(480);
        klatr::audio::IInputBuffer* inputBuffer = inputDevice->acquireInputBuffer();
        if (inputBuffer != nullptr && inputBuffer->empty()) {
            inputBuffer->release();
            inputBuffer = nullptr;
        }

        if (inputBuffer != nullptr && outputBuffer == nullptr) {
            std::printf("Input available: %u frames, output busy\n", inputBuffer->frameCount());
        }

        if (outputBuffer != nullptr) {
            float* output = reinterpret_cast<float*>(outputBuffer->map());
            if (inputBuffer != nullptr) {
                uint32_t inputAvailableFrames = inputDevice->currentPadding();
                uint32_t outputAvailableFrames = outputBuffer->frameCount() - outputDevice->currentPadding();

                float* input = reinterpret_cast<float*>(inputBuffer->map());

                uint32_t frames = std::min(inputAvailableFrames, outputAvailableFrames);
                uint32_t bytes = frames * 4 * 2;
                std::memcpy(&output[outputDevice->currentPadding() * 2], input, bytes);

                outputBuffer->produce(frames);
                inputBuffer->unmap();
                inputBuffer->consume();
            } else {
                constexpr float pi = 3.14159265358979323846f;

                float f = 480.0f;
                for (uint32_t i = 0; i < 100; i += 1) {
                    float t = static_cast<float>(i) / 48000.0f;
                    float v = std::sinf(2.0f * pi * f * t);
                    for (uint32_t ch = 0; ch < 2; ch += 1) {
                        output[i * 2 + ch] = v;
                    }
                }

                //outputBuffer->produce(100);
            }

            outputBuffer->unmap();
            outputBuffer->release();
        }

        if (inputBuffer != nullptr) {
            inputBuffer->release();
        }
    }

    inputDevice->stop(klatr::audio::DeviceFlowFlags::All);
    outputDevice->stop(klatr::audio::DeviceFlowFlags::All);

    outputDevice->release();
    inputDevice->release();
    instance->release();
    return 0;
}
*/
