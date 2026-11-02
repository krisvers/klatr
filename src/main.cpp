#include <klatr/gpu/context.hpp>

#include <vkom/dynlib.hpp>

#include <miniaudio.h>

#include <SDL3/SDL.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include "renderdoc_app.h"

/*
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Unknwn.h>
#include <dxcapi.h>
*/

#include <bit>
#include <bitset>
#include <fstream>
#include <cassert>
#include <chrono>
#include <functional>
#include <map>
#include <filesystem>
#include <mutex>
#include <semaphore>

//template<typename T, size_t N>
//class BoundedQueue {
//private:
//    T* _data;
//
//    size_t _producerOffset = 0;
//    size_t _consumerOffset = 0;
//
//    std::counting_semaphore<N> _full{0};
//    std::counting_semaphore<N> _empty{N};
//    std::mutex _lock = {};
//
//public:
//    BoundedQueue() {
//        _data = new T[N];
//    }
//
//    ~BoundedQueue() {
//        delete[] _data;
//    }
//
//    T const* data() const {
//        return _data;
//    }
//
//    T* data() {
//        return _data;
//    }
//
//    size_t capacity() const {
//        return N;
//    }
//
//    size_t producerOffset() const {
//        return _producerOffset;
//    }
//
//    size_t consumerOffset() const {
//        return _consumerOffset;
//    }
//
//    bool produce(size_t count, T const* data, bool blocking = true) {
//        if (blocking) {
//            _empty.acquire();
//        } else {
//            if (!_empty.try_acquire()) {
//                return false;
//            }
//        }
//
//        {
//            std::lock_guard<std::mutex> guard(_lock);
//
//            size_t copyRegions = ((_producerOffset % N) + count + N - 1) / N;
//            assert(copyRegions <= 2);
//
//            size_t firstCopyCount = std::min(N - (_producerOffset % N), count);
//            size_t secondCopyCount = count - firstCopyCount;
//
//            std::memcpy(&_data[_producerOffset % N], data, firstCopyCount * sizeof(T));
//            if (secondCopyCount != 0) {
//                std::memcpy(&_data[0], &data[firstCopyCount], secondCopyCount * sizeof(T));
//            }
//
//            _producerOffset += count;
//        }
//
//        _full.release();
//        return true;
//    }
//
//    size_t consume(size_t count, T* data, bool blocking = true) {
//        if (blocking) {
//            _full.acquire();
//        } else {
//            if (!_full.try_acquire()) {
//                return 0;
//            }
//        }
//
//        count = std::min(std::min(_producerOffset - _consumerOffset, N), count);
//
//        {
//            std::lock_guard<std::mutex> guard(_lock);
//
//            size_t copyRegions = ((_consumerOffset % N) + count + N - 1) / N;
//            assert(copyRegions <= 2);
//
//            size_t firstCopyCount = std::min(N - (_consumerOffset % N), count);
//            size_t secondCopyCount = count - firstCopyCount;
//
//            std::memcpy(&data[0], &_data[_consumerOffset % N], firstCopyCount * sizeof(T));
//            if (secondCopyCount != 0) {
//                std::memcpy(&data[firstCopyCount], &_data[0], secondCopyCount * sizeof(T));
//            }
//
//            _consumerOffset += count;
//        }
//
//        _empty.release();
//        return count;
//    }
//};
//
//enum class PassResourceMode {
//    Consume = 1,
//    Produce = 2,
//};
//
//enum class PassResourceFormat {
//    Float = 1,
//    Int32 = 2,
//};
//
//struct PassResourceAddressingInfo {
//    uint32_t slot;
//    char name[128];
//};
//
//struct PassResourceBounds {
//    float lower;
//    float upper;
//};
//
//struct PassSlotResourceInfo {
//    PassResourceAddressingInfo addressing;
//    PassResourceMode mode;
//    PassResourceFormat format;
//    PassResourceBounds defaultBounds;
//};
//
//struct PassSlotTileInfo {
//    uint32_t width;
//    uint32_t height;
//    uint32_t binding;
//    uint32_t set;
//    uint32_t count;
//    char name[128];
//};
//
//class IPass {
//public:
//    virtual bool enumerateResourceSlotInfos(uint32_t id, PassSlotResourceInfo* info) = 0;
//    virtual bool enumerateTileSlotInfos(uint32_t id, PassSlotTileInfo* info) = 0;
//
//    virtual bool prepare() = 0;
//
//    virtual bool bindResourceBySlot(uint32_t slot, uint64_t gpuAddress, uint32_t count, PassResourceBounds const* bounds = nullptr) = 0;
//    virtual bool bindResourceByName(const char* name, uint64_t gpuAddress, uint32_t count, PassResourceBounds const* bounds = nullptr) = 0;
//
//    virtual bool bindConstantBySlot(uint32_t slot, float f, int32_t i) = 0;
//    virtual bool bindConstantByName(const char* name, float f, int32_t i) = 0;
//
//    virtual bool bindTileByName(const char* name, uint32_t element, vkom::ITextureView* view) = 0;
//    virtual bool bindTileByBindingSet(uint32_t binding, uint32_t set, uint32_t element, vkom::ITextureView* view) = 0;
//
//    virtual bool execute(vkom::ICommandEncoder* encoder, uint32_t width, uint32_t height, uint32_t depth, vkom::IFence* executionFinishedFence) = 0;
//};
//
//#define INTERNAL_PLAYBACK_BUFFER_SAMPLE_COUNT 44100 * 10
//#define INTERNAL_AUDIO_BUFFER_FRAME_COUNT 16384

//struct UniformAudioBufferDescriptor {
//    uint32_t count;
//    uint32_t padding0;
//    float bounds[2]; /* note: if bounds[0] == bounds[1] this is treated as a constant */
//};
//
//struct Uniforms {
//    uint64_t globalID;
//    float globalTime;
//    uint32_t dispatchWidth;
//    uint32_t sampleRate;
//    float inverseSampleRate;
//};

#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_WRAPAROUND 0x0001
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_REVERSE 0x0002
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_MIRROR 0x0004

#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_WRAPAROUND 0x0010
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_NON_NORMALIZED 0x0020
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_FORCE_REINTERPRET 0x0040

#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_FORMAT_INT 0x0100

#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_ACCESS_READ 0x4000
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_ACCESS_WRITE 0x8000

enum class AudioBufferConfigFlags : uint32_t {
    IndexWraparound = 0x0001,
    IndexReverse = 0x0002,
    IndexMirror = 0x0004,

    ValueWraparound = 0x0010,
    ValueNonNormalized = 0x0020,
    ValueForceReinterpret = 0x0040,

    FormatInt = 0x0100,

    AccessRead = 0x4000,
    AccessWrite = 0x8000,
};

VKOM_DEFINE_ENUM_BITFLAGS_OPERATORS(AudioBufferConfigFlags)

struct PushConstantAudioBufferDescriptor {
    uint64_t gpuAddress = 0;
    uint32_t count = 0;

    AudioBufferConfigFlags config = {};

    /* if lowerBound == upperBound, they are treated as a read constant */
    float lowerBound = -1.0f;
    float upperBound = 1.0f;

    PushConstantAudioBufferDescriptor() = default;

    PushConstantAudioBufferDescriptor(uint64_t gpuAddress, uint32_t count, AudioBufferConfigFlags config, float lowerBound = -1.0f, float upperBound = 1.0f) : gpuAddress(gpuAddress), count(count), config(config), lowerBound(lowerBound), upperBound(upperBound) {}

    PushConstantAudioBufferDescriptor(vkom::IDeviceAddressBuffer* buffer, AudioBufferConfigFlags config, float lowerBound = -1.0f, float upperBound = 1.0f) : gpuAddress(buffer->deviceAddress()), config(config), lowerBound(lowerBound), upperBound(upperBound) {
        vkom::BufferInfo bufferInfo = {};
        buffer->getInfo(&bufferInfo);

        count = bufferInfo.size / sizeof(float);
    }
};

struct PushConstants {
    uint32_t sampleRate = 0;
    uint32_t reserved = 0;
    uint64_t dispatchSampleIndex = 0;
    PushConstantAudioBufferDescriptor buffers[4] = {};
};

//using PlaybackBuffer = BoundedQueue<float, INTERNAL_PLAYBACK_BUFFER_SAMPLE_COUNT>;

/* adapted from old test code from krisvers/vkom */
template<typename T>
std::vector<T> loadFile(const char* path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.good()) {
        return {};
    }

    size_t byteSize = in.tellg();
    if ((byteSize % sizeof(T)) != 0) {
        in.close();
        return {};
    }

    std::vector<T> code(byteSize / sizeof(T));
    in.seekg(0, std::ios::beg);

    if (!in.read(reinterpret_cast<char*>(&code[0]), byteSize)) {
        in.close();
        return {};
    }

    in.close();
    return code;
}

struct ModuleInfo {
    std::filesystem::file_time_type lastModified = std::filesystem::file_time_type::min();
    std::vector<uint32_t> spirv = {};
    vkom::IShaderModule* shader = nullptr;
};

vkom::IShaderModule* loadModule(ModuleInfo& info, vkom::IDevice* device, const char* path) {
    if (!std::filesystem::exists(path)) {
        return nullptr;
    }

    auto lastModified = std::filesystem::last_write_time(path);
    if (lastModified <= info.lastModified) {
        return nullptr;
    }

    info.spirv = loadFile<uint32_t>(path);
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

class KlatrGLSLangStaticIncluder : public glslang::TShader::Includer {
private:
    std::unordered_map<std::string, std::string> _systemFiles;
    std::unordered_map<std::string, std::string> _localFiles;

public:
    KlatrGLSLangStaticIncluder(std::unordered_map<std::string, std::string> systemFiles, std::unordered_map<std::string, std::string> localFiles) : _systemFiles(systemFiles), _localFiles(localFiles) {}
    ~KlatrGLSLangStaticIncluder() {}

    IncludeResult* includeSystem(const char* header, const char* includer, size_t depth) override {
        auto it = _systemFiles.find(header);
        if (it == _systemFiles.end()) {
            return nullptr;
        }

        return new IncludeResult(it->first, it->second.c_str(), it->second.size(), this);
    }

    IncludeResult* includeLocal(const char* header, const char* includer, size_t depth) override {
        auto it = _localFiles.find(header);
        if (it == _localFiles.end()) {
            return nullptr;
        }

        return new IncludeResult(it->first, it->second.c_str(), it->second.size(), this);
    }

    void releaseInclude(IncludeResult* result) override {
        delete result;
    }
};

vkom::IShaderModule* compileModule(vkom::IDevice* device, const char* source, size_t sourceLength, const char* entryPoint, vkom::ShaderStageFlags stage, std::unordered_map<std::string, std::string> const& systemIncludeFiles = {}, std::unordered_map<std::string, std::string> const& localIncludeFiles = {}) {
    int sourceLengthInt = static_cast<int>(sourceLength);

    const char* name = "module.hlsl";

    glslang::TShader shader = glslang::TShader(EShLangCompute);
    shader.setStringsWithLengthsAndNames(&source, &sourceLengthInt, &name, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, EShLangCompute, glslang::EShClientVulkan, 120);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_5);

    shader.setPreamble("#extension GL_GOOGLE_include_directive : require\n");

    vkom::AdapterLimits adapterLimits;
    device->parent<vkom::IAdapter>()->queryLimits(&adapterLimits);

    TBuiltInResource builtInResource = {};
    builtInResource.maxLights = 32;
    builtInResource.maxClipPlanes = 6;
    builtInResource.maxTextureUnits = 32;
    builtInResource.maxTextureCoords = 32;
    builtInResource.maxVertexAttribs = 64;
    builtInResource.maxVertexUniformComponents = 4096;
    builtInResource.maxVaryingFloats = 64;
    builtInResource.maxVertexTextureImageUnits = 32;
    builtInResource.maxCombinedTextureImageUnits = 80;
    builtInResource.maxTextureImageUnits = 32;
    builtInResource.maxFragmentUniformComponents = 4096;
    builtInResource.maxDrawBuffers = 32;
    builtInResource.maxVertexUniformVectors = 128;
    builtInResource.maxVaryingVectors = 8;
    builtInResource.maxFragmentUniformVectors = 16;
    builtInResource.maxVertexOutputVectors = 16;
    builtInResource.maxFragmentInputVectors = 15;
    builtInResource.minProgramTexelOffset = -8;
    builtInResource.maxProgramTexelOffset = 7;
    builtInResource.maxClipDistances = 8;
    builtInResource.maxComputeWorkGroupCountX = 65535;
    builtInResource.maxComputeWorkGroupCountY = 65535;
    builtInResource.maxComputeWorkGroupCountZ = 65535;
    builtInResource.maxComputeWorkGroupSizeX = 1024;
    builtInResource.maxComputeWorkGroupSizeY = 1024;
    builtInResource.maxComputeWorkGroupSizeZ = 64;
    builtInResource.maxComputeUniformComponents = 1024;
    builtInResource.maxComputeTextureImageUnits = 16;
    builtInResource.maxComputeImageUniforms = 8;
    builtInResource.maxComputeAtomicCounters = 8;
    builtInResource.maxComputeAtomicCounterBuffers = 1;
    builtInResource.maxVaryingComponents = 60;
    builtInResource.maxVertexOutputComponents = 64;
    builtInResource.maxGeometryInputComponents = 64;
    builtInResource.maxGeometryOutputComponents = 128;
    builtInResource.maxFragmentInputComponents = 128;
    builtInResource.maxImageUnits = 8;
    builtInResource.maxCombinedImageUnitsAndFragmentOutputs = 8;
    builtInResource.maxCombinedShaderOutputResources = 8;
    builtInResource.maxImageSamples = 0;
    builtInResource.maxVertexImageUniforms = 0;
    builtInResource.maxTessControlImageUniforms = 0;
    builtInResource.maxTessEvaluationImageUniforms = 0;
    builtInResource.maxGeometryImageUniforms = 0;
    builtInResource.maxFragmentImageUniforms = 8;
    builtInResource.maxCombinedImageUniforms = 8;
    builtInResource.maxGeometryTextureImageUnits = 16;
    builtInResource.maxGeometryOutputVertices = 256;
    builtInResource.maxGeometryTotalOutputComponents = 1024;
    builtInResource.maxGeometryUniformComponents = 1024;
    builtInResource.maxGeometryVaryingComponents = 64;
    builtInResource.maxTessControlInputComponents = 128;
    builtInResource.maxTessControlOutputComponents = 128;
    builtInResource.maxTessControlTextureImageUnits = 16;
    builtInResource.maxTessControlUniformComponents = 1024;
    builtInResource.maxTessControlTotalOutputComponents = 4096;
    builtInResource.maxTessEvaluationInputComponents = 128;
    builtInResource.maxTessEvaluationOutputComponents = 128;
    builtInResource.maxTessEvaluationTextureImageUnits = 16;
    builtInResource.maxTessEvaluationUniformComponents = 1024;
    builtInResource.maxTessPatchComponents = 120;
    builtInResource.maxPatchVertices = 32;
    builtInResource.maxTessGenLevel = 64;
    builtInResource.maxViewports = 16;
    builtInResource.maxVertexAtomicCounters = 0;
    builtInResource.maxTessControlAtomicCounters = 0;
    builtInResource.maxTessEvaluationAtomicCounters = 0;
    builtInResource.maxGeometryAtomicCounters = 0;
    builtInResource.maxFragmentAtomicCounters = 8;
    builtInResource.maxCombinedAtomicCounters = 8;
    builtInResource.maxAtomicCounterBindings = 1;
    builtInResource.maxVertexAtomicCounterBuffers = 0;
    builtInResource.maxTessControlAtomicCounterBuffers = 0;
    builtInResource.maxTessEvaluationAtomicCounterBuffers = 0;
    builtInResource.maxGeometryAtomicCounterBuffers = 0;
    builtInResource.maxFragmentAtomicCounterBuffers = 1;
    builtInResource.maxCombinedAtomicCounterBuffers = 1;
    builtInResource.maxAtomicCounterBufferSize = 16384;
    builtInResource.maxTransformFeedbackBuffers = 4;
    builtInResource.maxTransformFeedbackInterleavedComponents = 64;
    builtInResource.maxCullDistances = 8;
    builtInResource.maxCombinedClipAndCullDistances = 8;
    builtInResource.maxSamples = 4;
    builtInResource.maxMeshOutputVerticesNV = 256;
    builtInResource.maxMeshOutputPrimitivesNV = 512;
    builtInResource.maxMeshWorkGroupSizeX_NV = 32;
    builtInResource.maxMeshWorkGroupSizeY_NV = 1;
    builtInResource.maxMeshWorkGroupSizeZ_NV = 1;
    builtInResource.maxTaskWorkGroupSizeX_NV = 32;
    builtInResource.maxTaskWorkGroupSizeY_NV = 1;
    builtInResource.maxTaskWorkGroupSizeZ_NV = 1;
    builtInResource.maxMeshViewCountNV = 4;
    builtInResource.maxDualSourceDrawBuffersEXT = 1;
    builtInResource.limits.nonInductiveForLoops = 1;
    builtInResource.limits.whileLoops = 1;
    builtInResource.limits.doWhileLoops = 1;
    builtInResource.limits.generalUniformIndexing = 1;
    builtInResource.limits.generalAttributeMatrixVectorIndexing = 1;
    builtInResource.limits.generalVaryingIndexing = 1;
    builtInResource.limits.generalSamplerIndexing = 1;
    builtInResource.limits.generalVariableIndexing = 1;
    builtInResource.limits.generalConstantMatrixVectorIndexing = 1;

    EShMessages messages = EShMessages(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
    KlatrGLSLangStaticIncluder includer = { systemIncludeFiles, localIncludeFiles };
    bool success = shader.parse(&builtInResource, 450, ENoProfile, false, false, messages, includer);

    const char* log = shader.getInfoLog();
    if (log != nullptr && log[0] != '\0') {
        std::printf("info: %s\n", log);
    }

    log = shader.getInfoDebugLog();
    if (log != nullptr && log[0] != '\0') {
        std::printf("debug: %s\n", log);
    }

    if (!success) {
        return nullptr;
    }

    glslang::TIntermediate* intermediate = shader.getIntermediate();
    assert(intermediate != nullptr);

    glslang::TProgram program = {};
    program.addShader(&shader);
    success = program.link(messages);
    
    log = program.getInfoLog();
    if (log != nullptr && log[0] != '\0') {
        std::printf("info: %s\n", log);
    }

    log = program.getInfoDebugLog();
    if (log != nullptr && log[0] != '\0') {
        std::printf("debug: %s\n", log);
    }

    program.mapIO();

    spv::SpvBuildLogger logger = {};
    glslang::SpvOptions options = {};
    options.generateDebugInfo = true;

    std::vector<unsigned> spirvUnsigned;
    glslang::GlslangToSpv(*intermediate, spirvUnsigned, &logger, &options);

    std::string spirvLog = logger.getAllMessages();
    if (!spirvLog.empty()) {
        std::printf("spirv: %s\n", spirvLog.c_str());
    }

    vkom::ShaderModuleInfo moduleShaderInfo = {};
    moduleShaderInfo.length = spirvUnsigned.size();
    moduleShaderInfo.spirv = reinterpret_cast<uint32_t const*>(spirvUnsigned.data());

    vkom::IShaderModule* sh;
    if (device->createShaderModule(&moduleShaderInfo, &sh) != vkom::Result::Success) {
        return nullptr;
    }

    return sh;
}

/*
vkom::IShaderModule* compileModuleHLSL(vkom::IDevice* device, const char* source, size_t sourceLength, const char* entryPoint, vkom::ShaderStageFlags stageFlags) {
    IDxcUtils* dxcUtils;
    assert(SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils))));

    IDxcCompiler3* dxcCompiler;
    assert(SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler))));

    WCHAR target[7] = L"_s_6_0";

    int entryPointWideRequiredCount = MultiByteToWideChar(DXC_CP_UTF8, 0, entryPoint, -1, nullptr, 0);
    std::wstring entryPointWide = std::wstring(entryPointWideRequiredCount, L'\0');
    MultiByteToWideChar(DXC_CP_UTF8, 0, entryPoint, -1, &entryPointWide[0], entryPointWideRequiredCount);

    LPCWSTR arguments[] = {
        L"-T", target,
        L"-E", entryPointWide.c_str(),
        L"-spirv",
        //L"/enable-16bit-types",
        L"-Zi",
    };

    uint32_t argumentCount = sizeof(arguments) / sizeof(LPCWSTR) - 1;
    #ifndef NDEBUG
    argumentCount += 1;
    #endif

    switch (stageFlags) {
        case vkom::ShaderStageFlags::Vertex:
            target[0] = L'v';
            break;
        case vkom::ShaderStageFlags::Fragment:
            target[0] = L'p';
            break;
        case vkom::ShaderStageFlags::Compute:
            target[0] = L'c';
            break;
        case vkom::ShaderStageFlags::Geometry:
            target[0] = L'g';
            break;
        case vkom::ShaderStageFlags::Hull:
            target[0] = L'h';
            break;
        case vkom::ShaderStageFlags::Domain:
            target[0] = L'd';
            break;
        default:
            dxcCompiler->Release();
            dxcUtils->Release();
            return nullptr;
    }

    DxcBuffer sourceBuffer = {};
    sourceBuffer.Size = sourceLength;
    sourceBuffer.Ptr = source;
    sourceBuffer.Encoding = DXC_CP_UTF8;

    IDxcResult* dxcResult;
    if (FAILED(dxcCompiler->Compile(&sourceBuffer, arguments, argumentCount, nullptr, IID_PPV_ARGS(&dxcResult)))) {
        dxcCompiler->Release();
        dxcUtils->Release();
        return nullptr;
    }

    auto handleError = [](IDxcResult* result) {
        IDxcBlobEncoding* blob;
        if (result->GetErrorBuffer(&blob) != 0) {
            std::printf("DXC compilation failed (no error message available)");
            return;
        }

        BOOL known;
        uint32_t encoding;
        if (blob->GetEncoding(&known, &encoding) != 0 || !known || encoding != DXC_CP_UTF8 && encoding != DXC_CP_UTF16) {
            std::printf("DXC compilation failed (no error message available)");
            return;
        }

        if (encoding == DXC_CP_UTF8) {
            std::string message = std::string(reinterpret_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
            std::printf("DXC compilation error: %s", message.c_str());
        } else if (encoding == DXC_CP_UTF16) {
            std::wstring wmessage = std::wstring(reinterpret_cast<const wchar_t*>(blob->GetBufferPointer()), blob->GetBufferSize());
            std::printf("DXC compilation error: %ws", wmessage.c_str());
        }
    };

    HRESULT hresult;
    if (FAILED(dxcResult->GetStatus(&hresult)) && FAILED(hresult)) {
        handleError(dxcResult);
        dxcCompiler->Release();
        dxcUtils->Release();
        return nullptr;
    }

    IDxcBlob* compiledBlob;
    if (FAILED(dxcResult->GetResult(&compiledBlob))) {
        handleError(dxcResult);
        dxcCompiler->Release();
        dxcUtils->Release();
        return nullptr;
    }

    if (compiledBlob == nullptr || compiledBlob->GetBufferPointer() == nullptr || compiledBlob->GetBufferSize() == 0) {
        handleError(dxcResult);
        dxcCompiler->Release();
        dxcUtils->Release();
        return nullptr;
    }

    if (compiledBlob->GetBufferSize() % 4 == 0) {
        handleError(dxcResult);
        dxcCompiler->Release();
        dxcUtils->Release();
        return nullptr;
    }

    std::vector<uint32_t> spirv(compiledBlob->GetBufferSize() / sizeof(uint32_t));
    std::memcpy(&spirv[0], compiledBlob->GetBufferPointer(), compiledBlob->GetBufferSize());

    dxcCompiler->Release();
    dxcUtils->Release();
    compiledBlob->Release();

    vkom::ShaderModuleInfo moduleShaderInfo = {};
    moduleShaderInfo.length = spirv.size();
    moduleShaderInfo.spirv = spirv.data();

    vkom::IShaderModule* shader;
    if (device->createShaderModule(&moduleShaderInfo, &shader) != vkom::Result::Success) {
        return nullptr;
    }

    return shader;
}
*/

vkom::IComputePipeline* compileComputePipeline(vkom::IDevice* device, vkom::IPipelineLayout* layout, const char* source, size_t sourceLength, std::unordered_map<std::string, std::string> const& systemIncludeFiles = {}, std::unordered_map<std::string, std::string> const& localIncludeFiles = {}) {
    vkom::IShaderModule* shader = compileModule(device, source, sourceLength, "main", vkom::ShaderStageFlags::Compute, systemIncludeFiles, localIncludeFiles);
    if (shader == nullptr) {
        return nullptr;
    }

    vkom::ComputePipelineInfo pipelineInfo = {};
    pipelineInfo.shaderInfo.shader = shader;
    pipelineInfo.shaderInfo.stage = vkom::ShaderStageFlags::Compute;
    pipelineInfo.shaderInfo.entry = "main";

    vkom::IComputePipeline* pipeline;
    if (device->createComputePipeline(&pipelineInfo, nullptr, layout, &pipeline) != vkom::Result::Success) {
        shader->release();
        return nullptr;
    }

    shader->release();
    return pipeline;
}

vkom::IComputePipeline* loadPipelineFromFile(ModuleInfo& moduleInfo, vkom::IDevice* device, vkom::IPipelineLayout* layout, const char* path) {
    if (loadModule(moduleInfo, device, path) == nullptr) {
        return nullptr;
    }

    vkom::ComputePipelineInfo modulePipelineInfo = {};
    modulePipelineInfo.shaderInfo.shader = moduleInfo.shader;
    modulePipelineInfo.shaderInfo.stage = vkom::ShaderStageFlags::Compute;
    modulePipelineInfo.shaderInfo.entry = "main";

    vkom::IComputePipeline* modulePipeline;
    if (device->createComputePipeline(&modulePipelineInfo, nullptr, layout, &modulePipeline) != vkom::Result::Success) {
        return nullptr;
    }

    return modulePipeline;
}

//struct GeneralPassPipelineData {
//    vkom::IPipelineLayout* layout;
//    vkom::IComputePipeline* pipeline;
//};
//
//struct GeneralPassDispatchData {
//    vkom::IFence* fence;
//    std::unordered_map<uint32_t, vkom::IDescriptorSet*> sets;
//};
//
//struct GeneralPassResourceBindingData {
//    uint64_t gpuAddress = 0;
//    uint32_t count = 0;
//    PassResourceBounds bounds;
//    float f = 0.0f;
//    int32_t i = 0;
//};
//
//struct GeneralPassBindingSet {
//    uint32_t binding;
//    uint32_t set;
//
//    bool operator==(GeneralPassBindingSet const& other) const noexcept {
//        return (other.binding == binding) && (other.set == set);
//    }
//
//    bool operator<(GeneralPassBindingSet const& other) const noexcept {
//        if (set != other.set) {
//            return set < other.set;
//        }
//
//        return binding < other.binding;
//    }
//};
//
//namespace std {
//
//template<>
//struct hash<GeneralPassBindingSet> {
//    size_t operator()(GeneralPassBindingSet const& bindingSet) const noexcept {
//        size_t h1 = std::hash<uint32_t>{}(bindingSet.binding);
//        size_t h2 = std::hash<uint32_t>{}(bindingSet.set);
//
//        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
//    }
//};
//
//}
//
//class GeneralPass : public IPass {
//private:
//    klatr::gpu::Context const& _context;
//    GeneralPassPipelineData _pipelineData = {};
//
//    /* metadata */
//    std::map<uint32_t, PassSlotResourceInfo> _resourceInfos = {};
//    std::unordered_map<std::string, uint32_t> _resourceNamesToSlot = {};
//    std::map<GeneralPassBindingSet, PassSlotTileInfo> _tileInfos = {};
//    std::unordered_map<std::string, GeneralPassBindingSet> _tileNamesToBindingSet = {};
//
//    /* descriptor creation */
//    bool _isDescriptorless = false;
//    std::unordered_map<uint32_t, vkom::IDescriptorSetLayout*> _descriptorSetLayouts = {};
//    vkom::IDescriptorPool* _descriptorPool = nullptr;
//
//    /* per-dispatch descriptor management */
//    uint32_t _dispatchesInFlight = 1;
//    bool _blocking = false;
//    uint32_t _dispatchID = 0;
//    std::vector<GeneralPassDispatchData> _dispatchDatas = {};
//
//    /* per-dispatch bindings */
//    std::unordered_map<uint32_t, GeneralPassResourceBindingData> _resourceBindings = {};
//    std::unordered_map<GeneralPassBindingSet, std::vector<vkom::ITextureView*>> _tileBindings = {};
//
//public:
//    GeneralPass(klatr::gpu::Context const& context, GeneralPassPipelineData const& pipelineData, std::vector<PassSlotResourceInfo> const& resourceInfos, std::vector<PassSlotTileInfo> const& tileInfos, uint32_t dispatchesInFlight = 1, bool blocking = true) : _context(context), _pipelineData(pipelineData), _dispatchesInFlight(std::max(1u, dispatchesInFlight)), _blocking(blocking) {
//        _pipelineData.layout->retain();
//        _pipelineData.pipeline->retain();
//
//        for (PassSlotResourceInfo const& info : resourceInfos) {
//            _resourceInfos[info.addressing.slot] = info;
//            if (info.addressing.name[0] != '\0') {
//                _resourceNamesToSlot[info.addressing.name] = info.addressing.slot;
//            }
//        }
//
//        for (PassSlotTileInfo const& info : tileInfos) {
//            GeneralPassBindingSet bindingSet = {};
//            bindingSet.binding = info.binding;
//            bindingSet.set = info.set;
//
//            _tileInfos[bindingSet] = info;
//            if (info.name[0] != '\0') {
//                _tileNamesToBindingSet[info.name] = bindingSet;
//            }
//        }
//
//        _isDescriptorless = _tileInfos.empty();
//        if (!_isDescriptorless) {
//            std::vector<vkom::DescriptorPoolDescriptorInfo> descriptorInfos = {};
//            std::unordered_map<uint32_t, std::vector<vkom::DescriptorBindingInfo>> setBindingInfos = {};
//            for (auto tilePair : _tileInfos) {
//                auto it = setBindingInfos.find(tilePair.first.set);
//                if (it == setBindingInfos.end()) {
//                    setBindingInfos[tilePair.first.set] = {};
//                    it = setBindingInfos.find(tilePair.first.set);
//                }
//
//                for (vkom::DescriptorBindingInfo const& bindingInfo : it->second) {
//                    /* TODO: error handling for duplicate bindings */
//                    assert(bindingInfo.binding != tilePair.first.binding);
//                }
//
//                vkom::DescriptorBindingInfo bindingInfo = {};
//                bindingInfo.binding = tilePair.first.binding;
//                bindingInfo.flags = vkom::DescriptorFlags::StorageTexture;
//                bindingInfo.count = tilePair.second.count;
//                bindingInfo.stages = vkom::ShaderStageFlags::Compute;
//
//                it->second.push_back(bindingInfo);
//
//                bool foundDescriptorType = false;
//                for (vkom::DescriptorPoolDescriptorInfo& descriptorInfo : descriptorInfos) {
//                    if (descriptorInfo.flags == vkom::DescriptorFlags::StorageTexture) {
//                        descriptorInfo.count += tilePair.second.count;
//
//                        foundDescriptorType = true;
//                        break;
//                    }
//                }
//
//                if (!foundDescriptorType) {
//                    vkom::DescriptorPoolDescriptorInfo descriptorInfo = {};
//                    descriptorInfo.flags = vkom::DescriptorFlags::StorageTexture;
//                    descriptorInfo.count = tilePair.second.count;
//
//                    descriptorInfos.push_back(descriptorInfo);
//                }
//            }
//
//            for (auto pair : setBindingInfos) {
//                vkom::DescriptorSetLayoutInfo layoutInfo = {};
//                layoutInfo.bindingCount = static_cast<uint32_t>(pair.second.size());
//                layoutInfo.bindings = pair.second.data();
//
//                vkom::IDescriptorSetLayout* layout;
//                assert(_context.device->createDescriptorSetLayout(&layoutInfo, &layout) == vkom::Result::Success);
//
//                _descriptorSetLayouts[pair.first] = layout;
//            }
//
//            for (vkom::DescriptorPoolDescriptorInfo& descriptorInfo : descriptorInfos) {
//                descriptorInfo.count *= dispatchesInFlight;
//            }
//
//            vkom::DescriptorPoolInfo descriptorPoolInfo = {};
//            descriptorPoolInfo.maxDescriptorSets = setBindingInfos.size() * dispatchesInFlight;
//            descriptorPoolInfo.descriptorCount = static_cast<uint32_t>(descriptorInfos.size());
//            descriptorPoolInfo.descriptors = descriptorInfos.data();
//
//            assert(_context.device->createDescriptorPool(&descriptorPoolInfo, &_descriptorPool) == vkom::Result::Success);
//
//            _dispatchDatas.resize(dispatchesInFlight);
//            for (auto pair : setBindingInfos) {
//                std::vector<vkom::IDescriptorSet*> sets(dispatchesInFlight);
//                assert(_descriptorPool->allocateDescriptorSets(_descriptorSetLayouts[pair.first], static_cast<uint32_t>(sets.size()), &sets[0]) == vkom::Result::Success);
//
//                for (uint32_t i = 0; i < dispatchesInFlight; i += 1) {
//                    _dispatchDatas[i].fence = nullptr;
//                    _dispatchDatas[i].sets[pair.first] = sets[i];
//                }
//            }
//        }
//    }
//
//    ~GeneralPass() {
//        for (GeneralPassDispatchData const& dispatch : _dispatchDatas) {
//            if (dispatch.fence != nullptr) {
//                dispatch.fence->release();
//            }
//
//            for (auto pair : dispatch.sets) {
//                pair.second->release();
//            }
//        }
//
//        for (auto pair : _descriptorSetLayouts) {
//            pair.second->release();
//        }
//
//        if (_descriptorPool != nullptr) {
//            _descriptorPool->release();
//        }
//
//        _pipelineData.layout->release();
//        _pipelineData.pipeline->release();
//    }
//
//    bool enumerateResourceSlotInfos(uint32_t id, PassSlotResourceInfo* info) override {
//        auto it = std::next(_resourceInfos.begin(), id);
//        if (it == _resourceInfos.end()) {
//            return false;
//        }
//
//        *info = it->second;
//        return true;
//    }
//
//    bool enumerateTileSlotInfos(uint32_t id, PassSlotTileInfo* info) override {
//        auto it = std::next(_tileInfos.begin(), id);
//        if (it == _tileInfos.end()) {
//            return false;
//        }
//
//        *info = it->second;
//        return true;
//    }
//
//    bool prepare() override {
//        return true;
//    }
//
//    bool bindResourceBySlot(uint32_t slot, uint64_t gpuAddress, uint32_t count, PassResourceBounds const* bounds = nullptr) override {
//        if (!_resourceInfos.contains(slot)) {
//            return false;
//        }
//
//        _resourceBindings[slot] = {};
//        _resourceBindings[slot].gpuAddress = gpuAddress;
//        _resourceBindings[slot].count = count;
//
//        if (bounds != nullptr) {
//            _resourceBindings[slot].bounds = *bounds;
//        } else {
//            _resourceBindings[slot].bounds = _resourceInfos[slot].defaultBounds;
//        }
//
//        return true;
//    }
//
//    bool bindResourceByName(const char* name, uint64_t gpuAddress, uint32_t count, PassResourceBounds const* bounds = nullptr) override {
//        auto it = _resourceNamesToSlot.find(name);
//        if (it == _resourceNamesToSlot.end()) {
//            return false;
//        }
//
//        return bindResourceBySlot(it->second, gpuAddress, count, bounds);
//    }
//
//    bool bindConstantBySlot(uint32_t slot, float f, int32_t i) override {
//        if (!_resourceInfos.contains(slot)) {
//            return false;
//        }
//
//        _resourceBindings[slot] = {};
//        _resourceBindings[slot].f = f;
//        _resourceBindings[slot].i = i;
//
//        return true;
//    }
//
//    bool bindConstantByName(const char* name, float f, int32_t i) override {
//        auto it = _resourceNamesToSlot.find(name);
//        if (it == _resourceNamesToSlot.end()) {
//            return false;
//        }
//
//        return bindConstantBySlot(it->second, f, i);
//    }
//
//    bool bindTileByBindingSet(uint32_t binding, uint32_t set, uint32_t element, vkom::ITextureView* view) override {
//        vkom::ITexture* texture = view->parent<vkom::ITexture>();
//        if (texture == nullptr) {
//            return false;
//        }
//
//        vkom::IStorageTexture* storageTexture = texture->queryInterface<vkom::IStorageTexture>();
//        if (storageTexture == nullptr) {
//            return false;
//        }
//
//        GeneralPassBindingSet bindingSet = {};
//        bindingSet.binding = binding;
//        bindingSet.set = set;
//
//        if (!_tileInfos.contains(bindingSet)) {
//            return false;
//        }
//
//        if (element >= _tileInfos[bindingSet].count) {
//            return false;
//        }
//
//        if (!_tileBindings.contains(bindingSet)) {
//            _tileBindings[bindingSet] = std::vector<vkom::ITextureView*>(_tileInfos[bindingSet].count);
//        }
//
//        _tileBindings[bindingSet][element] = view;
//        return true;
//    }
//
//    bool bindTileByName(const char* name, uint32_t element, vkom::ITextureView* view) override {
//        auto it = _tileNamesToBindingSet.find(name);
//        if (it == _tileNamesToBindingSet.end()) {
//            return false;
//        }
//
//        return bindTileByBindingSet(it->second.binding, it->second.set, element, view);
//    }
//
//    bool execute(vkom::ICommandEncoder* encoder, uint32_t width, uint32_t height, uint32_t depth, vkom::IFence* executionFinishedFence) override {
//        if (!_isDescriptorless) {
//            GeneralPassDispatchData& currentDispatch = _dispatchDatas[_dispatchID % _dispatchesInFlight];
//            if (currentDispatch.fence != nullptr && !currentDispatch.fence->status()) {
//                if (!_blocking) {
//                    return false;
//                }
//
//                assert(currentDispatch.fence->wait() == vkom::Result::Success);
//                currentDispatch.fence->release();
//            }
//
//            currentDispatch.fence = executionFinishedFence;
//            currentDispatch.fence->retain();
//
//            size_t totalDescriptorCount = 0;
//
//            for (auto tilePair : _tileBindings) {
//                totalDescriptorCount += tilePair.second.size();
//            }
//
//            vkom::DescriptorTextureInfo sharedTextureInfo = {};
//            sharedTextureInfo.layout = vkom::TextureLayout::General;
//
//            std::vector<vkom::DescriptorWrite> descriptorWrites(_tileBindings.size());
//            std::vector<vkom::IResourceView*> totalDescriptorViews(totalDescriptorCount);
//            std::vector<vkom::DescriptorTextureInfo> totalDescriptorTextureInfos(totalDescriptorCount, sharedTextureInfo);
//
//            uint32_t previousSet = 0;
//            size_t previousSetInitialWriteIndex = 0;
//
//            size_t currentWriteIndex = 0;
//            size_t currentDescriptorIndex = 0;
//            for (auto tilePair : _tileBindings) {
//                if (previousSet != tilePair.first.set && currentWriteIndex != previousSetInitialWriteIndex) {
//                    currentDispatch.sets[previousSet]->write(currentWriteIndex - previousSetInitialWriteIndex, descriptorWrites.data() + previousSetInitialWriteIndex);
//
//                    previousSet = tilePair.first.set;
//                    previousSetInitialWriteIndex = currentWriteIndex;
//                }
//
//                for (size_t i = 0; i < tilePair.second.size(); i += 1) {
//                    totalDescriptorViews[currentDescriptorIndex + i] = tilePair.second[i]->queryInterface<vkom::IResourceView>();
//                }
//
//                vkom::DescriptorWrite write = {};
//                write.binding = tilePair.first.binding;
//                write.element = 0;
//                write.count = static_cast<uint32_t>(tilePair.second.size());
//                write.views = totalDescriptorViews.data() + currentDescriptorIndex;
//                write.textureInfos = totalDescriptorTextureInfos.data() + currentDescriptorIndex;
//
//                descriptorWrites[currentWriteIndex] = write;
//
//                currentWriteIndex += 1;
//                currentDescriptorIndex += tilePair.second.size();
//            }
//
//            if (currentWriteIndex != previousSetInitialWriteIndex) {
//                currentDispatch.sets[previousSet]->write(currentWriteIndex - previousSetInitialWriteIndex, descriptorWrites.data() + previousSetInitialWriteIndex);
//            }
//        }
//
//        vkom::ComputePassDescriptor passDescriptor = {};
//        vkom::IComputePass* pass = encoder->beginComputePass(&passDescriptor);
//
//        pass->bindPipeline(_pipelineData.pipeline);
//        if (!_isDescriptorless) {
//            GeneralPassDispatchData& currentDispatch = _dispatchDatas[_dispatchID % _dispatchesInFlight];
//            for (auto pair : currentDispatch.sets) {
//                pass->bindDescriptorSet(_pipelineData.layout, pair.first, pair.second, 0, nullptr);
//            }
//        }
//
//        std::vector<PushConstantAudioBufferDescriptor> pushConstants = {};
//        for (auto binding : _resourceBindings) {
//            pushConstants.resize(std::max(pushConstants.size(), static_cast<size_t>(binding.first) + 1));
//
//            if (binding.second.gpuAddress != 0) {
//                pushConstants[binding.first].gpuAddress = binding.second.gpuAddress;
//                pushConstants[binding.first].count = binding.second.count;
//                pushConstants[binding.first].lowerBound = binding.second.bounds.lower;
//                pushConstants[binding.first].upperBound = binding.second.bounds.upper;
//
//                /* transition buffers */
//            } else {
//                pushConstants[binding.first].gpuAddress = 0;
//                pushConstants[binding.first].count = 0;
//                
//                if (_resourceInfos[binding.first].format == PassResourceFormat::Float) {
//                    pushConstants[binding.first].lowerBound = binding.second.f;
//                    pushConstants[binding.first].upperBound = binding.second.f;
//                } else {
//                    pushConstants[binding.first].lowerBound = std::bit_cast<float>(binding.second.i);
//                    pushConstants[binding.first].upperBound = std::bit_cast<float>(binding.second.i);
//                }
//            }
//        }
//
//        pass->pushConstants(_pipelineData.layout, vkom::ShaderStageFlags::Compute, 0, pushConstants.size() * sizeof(PushConstantAudioBufferDescriptor), pushConstants.data());
//
//        for (auto binding : _tileBindings) {
//            for (vkom::ITextureView* view : binding.second) {
//                vkom::ITexture* texture = view->parent<vkom::ITexture>();
//                assert(texture != nullptr);
//
//                vkom::TextureInfo textureInfo;
//                texture->getInfo(&textureInfo);
//
//                vkom::ITransferDestinationTexture* transferDestinationTexture = texture->queryInterface<vkom::ITransferDestinationTexture>();
//                if (transferDestinationTexture != nullptr) {
//                    vkom::TextureTransition acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition = {};
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.general.srcStage = vkom::PipelineStageFlags::TopOfPipe;
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.general.dstStage = vkom::PipelineStageFlags::Transfer;
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.transfer.oldFamily = _context.rasterGraphicsQueue->family();
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.transfer.newFamily = _context.audioComputeQueue->family();
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.oldLayout = vkom::TextureLayout::Undefined;
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.newLayout = vkom::TextureLayout::TransferDestination;
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.aspectFlags = vkom::TextureAspectFlags::Color;
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.subresourcePosition.layer = 0;
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.subresourcePosition.mip = 0;
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.subresourceDimensions.layers = textureInfo.dimensions.subresource.layers;
//                    acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition.subresourceDimensions.mips = textureInfo.dimensions.subresource.mips;
//
//                    encoder->transitionTexture(texture, &acquireOwnershipTopToTransferUndefinedToTransferDestinationTransition);
//
//                    vkom::ColorTextureClear clear = {};
//                    clear.layout = vkom::TextureLayout::General;
//                    clear.color[0] = 0.0f;
//                    clear.color[1] = 0.0f;
//                    clear.color[2] = 0.0f;
//                    clear.color[3] = 0.0f;
//                    clear.subresourceOffset.layer = 0;
//                    clear.subresourceOffset.mip = 0;
//                    clear.subresourceRange.layers = textureInfo.dimensions.subresource.layers;
//                    clear.subresourceRange.mips = textureInfo.dimensions.subresource.mips;
//
//                    encoder->clearColorTexture(transferDestinationTexture, &clear);
//
//                    vkom::TextureTransition transferToComputeTransferDestinationToGeneralTransition = {};
//                    transferToComputeTransferDestinationToGeneralTransition.general.srcStage = vkom::PipelineStageFlags::Transfer;
//                    transferToComputeTransferDestinationToGeneralTransition.general.dstStage = vkom::PipelineStageFlags::ComputeShader;
//                    transferToComputeTransferDestinationToGeneralTransition.general.srcAccess = vkom::ResourceAccessFlags::TransferWrite;
//                    transferToComputeTransferDestinationToGeneralTransition.general.dstAccess = vkom::ResourceAccessFlags::ShaderRead;
//                    transferToComputeTransferDestinationToGeneralTransition.transfer.oldFamily = _context.audioComputeQueue->family();
//                    transferToComputeTransferDestinationToGeneralTransition.transfer.newFamily = _context.audioComputeQueue->family();
//                    transferToComputeTransferDestinationToGeneralTransition.oldLayout = vkom::TextureLayout::TransferDestination;
//                    transferToComputeTransferDestinationToGeneralTransition.newLayout = vkom::TextureLayout::General;
//                    transferToComputeTransferDestinationToGeneralTransition.aspectFlags = vkom::TextureAspectFlags::Color;
//                    transferToComputeTransferDestinationToGeneralTransition.subresourcePosition.layer = 0;
//                    transferToComputeTransferDestinationToGeneralTransition.subresourcePosition.mip = 0;
//                    transferToComputeTransferDestinationToGeneralTransition.subresourceDimensions.layers = textureInfo.dimensions.subresource.layers;
//                    transferToComputeTransferDestinationToGeneralTransition.subresourceDimensions.mips = textureInfo.dimensions.subresource.mips;
//
//                    encoder->transitionTexture(texture, &transferToComputeTransferDestinationToGeneralTransition);
//                } else {
//                    vkom::TextureTransition acquireOwnershipTopToComputeUndefinedToGeneralTransition = {};
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.general.srcStage = vkom::PipelineStageFlags::TopOfPipe;
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.general.dstStage = vkom::PipelineStageFlags::ComputeShader;
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.transfer.oldFamily = _context.rasterGraphicsQueue->family();
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.transfer.newFamily = _context.audioComputeQueue->family();
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.oldLayout = vkom::TextureLayout::Undefined;
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.newLayout = vkom::TextureLayout::General;
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.aspectFlags = vkom::TextureAspectFlags::Color;
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.subresourcePosition.layer = 0;
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.subresourcePosition.mip = 0;
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.subresourceDimensions.layers = textureInfo.dimensions.subresource.layers;
//                    acquireOwnershipTopToComputeUndefinedToGeneralTransition.subresourceDimensions.mips = textureInfo.dimensions.subresource.mips;
//
//                    encoder->transitionTexture(texture, &acquireOwnershipTopToComputeUndefinedToGeneralTransition);
//                }
//            }
//        }
//
//        pass->dispatch(width, height, depth);
//
//        for (auto binding : _tileBindings) {
//            for (vkom::ITextureView* view : binding.second) {
//                vkom::ITexture* texture = view->parent<vkom::ITexture>();
//                assert(texture != nullptr);
//
//                vkom::TextureInfo textureInfo;
//                texture->getInfo(&textureInfo);
//
//                vkom::TextureTransition releaseOwnershipComputeToTransferGeneralToTransferReadTransition = {};
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.general.srcStage = vkom::PipelineStageFlags::ComputeShader;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.general.dstStage = vkom::PipelineStageFlags::Transfer;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.general.srcAccess = vkom::ResourceAccessFlags::ShaderWrite;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.general.dstAccess = vkom::ResourceAccessFlags::TransferRead;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.transfer.oldFamily = _context.audioComputeQueue->family();
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.transfer.newFamily = _context.rasterGraphicsQueue->family();
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.oldLayout = vkom::TextureLayout::General;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.newLayout = vkom::TextureLayout::TransferSource;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.aspectFlags = vkom::TextureAspectFlags::Color;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.subresourcePosition.layer = 0;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.subresourcePosition.mip = 0;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.subresourceDimensions.layers = textureInfo.dimensions.subresource.layers;
//                releaseOwnershipComputeToTransferGeneralToTransferReadTransition.subresourceDimensions.mips = textureInfo.dimensions.subresource.mips;
//
//                encoder->transitionTexture(texture, &releaseOwnershipComputeToTransferGeneralToTransferReadTransition);
//            }
//        }
//
//        pass->end();
//
//        _resourceBindings.clear();
//        _tileBindings.clear();
//
//        _dispatchID += 1;
//
//        return true;
//    }
//};
//
//vkom::SurfaceWSIInfo surfaceWSIInfoFromSDLWindow(SDL_Window* window) {
//    vkom::SurfaceWSIInfo info = {};
//
//    #ifdef VKOM_PLATFORM_FAMILY_NT
//    info.type = vkom::SurfaceWSIType::Win32;
//    info.windowHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
//    info.displayHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr));
//    #elif defined(VKOM_PLATFORM_FAMILY_APPLE)
//    SDL_MetalView view = SDL_Metal_CreateView(window);
//
//    info.type = vkom::SurfaceWSIType::Metal;
//    info.windowHandle = reinterpret_cast<uint64_t>(SDL_Metal_GetLayer(view));
//    #else
//    info.type = vkom::SurfaceWSIType::Xlib;
//    info.windowHandle = reinterpret_cast<uint64_t>(SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, nullptr));
//    info.displayHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));
//
//    if (info.windowHandle == 0) {
//        info.type = vkom::SurfaceWSIType::Wayland;
//        info.windowHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));
//        info.displayHandle = reinterpret_cast<uint64_t>(SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
//    }
//    #endif
//
//    return info;
//}

int main(int argc, char** argv) {
    vkom::IDynlib* renderdocDynlib = vkom::loadDynlib("renderdoc.dll");
    if (renderdocDynlib == nullptr) {
        renderdocDynlib = vkom::loadDynlib("librenderdoc.so");
    }

    RENDERDOC_API_1_7_0* renderdoc = nullptr;
    if (renderdocDynlib != nullptr) {
        pRENDERDOC_GetAPI getAPI = renderdocDynlib->loadSymbol<pRENDERDOC_GetAPI>("RENDERDOC_GetAPI");
        assert(getAPI != nullptr);

        assert(getAPI(eRENDERDOC_API_Version_1_7_0, reinterpret_cast<void**>(&renderdoc)) == 1);
    }

    klatr::gpu::Context gpuContext = klatr::gpu::Context(true);
    gpuContext.instance->setLogCallback([](vkom::IInstance* instance, void* userData, vkom::DebugMessageSeverityFlags severity, vkom::DebugMessageTypeFlags types, const char* message) {
        std::printf("[vkom]: %s\n", message);
    }, nullptr);

    assert(glslang::InitializeProcess());
    assert(ShInitialize());

    assert(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* mainWindow = SDL_CreateWindow("klatr | main", 1200, 800, 0);
    assert(mainWindow != nullptr);

    //vkom::IWSIInstance* wsiInstance = gpuContext.instance->queryInterface<vkom::IWSIInstance>();
    //assert(wsiInstance != nullptr);

    //vkom::IWSIDevice* wsiDevice = gpuContext.device->queryInterface<vkom::IWSIDevice>();
    //assert(wsiDevice != nullptr);

    /* TODO: other platforms */
    //vkom::SurfaceWSIInfo mainSurfaceInfo = surfaceWSIInfoFromSDLWindow(mainWindow);

    //vkom::ISurface* mainSurface;
    //assert(wsiInstance->createSurface(&mainSurfaceInfo, &mainSurface) == vkom::Result::Success);

    //vkom::SurfaceCapabilities mainSurfaceCapabilities = {};
    //gpuContext.adapter->querySurfaceCapabilities(mainSurface, &mainSurfaceCapabilities);

    //vkom::SwapchainInfo mainSwapchainInfo = {};
    //mainSwapchainInfo.backbufferCount = std::min(std::max(3u, mainSurfaceCapabilities.minBackbufferCount), mainSurfaceCapabilities.maxBackbufferCount);
    //mainSwapchainInfo.backbufferInfo.samplesPerTexel = 1;
    //mainSwapchainInfo.backbufferInfo.usage = vkom::TextureUsageFlags::TransferDestination | vkom::TextureUsageFlags::TransferSource;
    //mainSwapchainInfo.backbufferInfo.dimensions.extent = mainSurfaceCapabilities.currentExtent;
    //mainSwapchainInfo.backbufferInfo.dimensions.subresource.layers = 1;
    //mainSwapchainInfo.backbufferInfo.dimensions.subresource.mips = 1;
    //mainSwapchainInfo.backbufferInfo.location = vkom::MemoryLocationFlags::GPU;
    //mainSwapchainInfo.preTransform = vkom::SurfaceTransformFlags::Identity;
    //mainSwapchainInfo.compositeAlpha = vkom::CompositeAlphaFlags::Opaque;
    //mainSwapchainInfo.surfaceFormatBits = gpuContext.adapter->querySurfaceFormatBits(mainSurface, vkom::Format::RGBA8UnsignedNormSRGB, vkom::ColorSpaceFlags::All) | gpuContext.adapter->querySurfaceFormatBits(mainSurface, vkom::Format::RGBA8UnsignedNorm, vkom::ColorSpaceFlags::All) | gpuContext.adapter->querySurfaceFormatBits(mainSurface, vkom::Format::BGRA8UnsignedNormSRGB, vkom::ColorSpaceFlags::All) | gpuContext.adapter->querySurfaceFormatBits(mainSurface, vkom::Format::BGRA8UnsignedNorm, vkom::ColorSpaceFlags::All);
    //mainSwapchainInfo.presentModeFlags = gpuContext.adapter->querySurfacePresentModes(mainSurface);
    
    //vkom::ISwapchain* mainSwapchain;
    //assert(wsiDevice->createSwapchain(mainSurface, &mainSwapchainInfo, &mainSwapchain) == vkom::Result::Success);

    vkom::IHeap* cpuEndpointBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::StorageBuffer | vkom::BufferUsageFlags::ShaderDeviceAddress, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::CPU, &cpuEndpointBufferHeap) == vkom::Result::Success);

    vkom::IHeap* gpuTransientBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::StorageBuffer | vkom::BufferUsageFlags::ShaderDeviceAddress, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::GPU, &gpuTransientBufferHeap) == vkom::Result::Success);

    vkom::IHeap* cpuUniformBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::UniformBuffer, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::CPU, &cpuUniformBufferHeap) == vkom::Result::Success);

    //vkom::IHeap* gpuModuleTileTextureHeap;
    //assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::None, vkom::TextureUsageFlags::TransferDestination | vkom::TextureUsageFlags::TransferSource | vkom::TextureUsageFlags::Storage, vkom::MemoryLocationFlags::GPU, &gpuModuleTileTextureHeap) == vkom::Result::Success);

    //vkom::DescriptorBindingInfo moduleCommonUniformsDescriptorSetLayoutBindingInfos[1] = {};
    //moduleCommonUniformsDescriptorSetLayoutBindingInfos[0].binding = 0;
    //moduleCommonUniformsDescriptorSetLayoutBindingInfos[0].flags = vkom::DescriptorFlags::UniformBuffer;
    //moduleCommonUniformsDescriptorSetLayoutBindingInfos[0].count = 1;
    //moduleCommonUniformsDescriptorSetLayoutBindingInfos[0].stages = vkom::ShaderStageFlags::Compute;

    //vkom::DescriptorSetLayoutInfo moduleCommonUniformsDescriptorSetLayoutInfo = {};
    //moduleCommonUniformsDescriptorSetLayoutInfo.bindingCount = 1;
    //moduleCommonUniformsDescriptorSetLayoutInfo.bindings = &moduleCommonUniformsDescriptorSetLayoutBindingInfos[0];

    //vkom::IDescriptorSetLayout* moduleCommonUniformsDescriptorSetLayout;
    //assert(gpuContext.device->createDescriptorSetLayout(&moduleCommonUniformsDescriptorSetLayoutInfo, &moduleCommonUniformsDescriptorSetLayout) == vkom::Result::Success);

    //vkom::DescriptorBindingInfo moduleTileDescriptorSetLayoutBindingInfos[1] = {};
    //moduleTileDescriptorSetLayoutBindingInfos[0].binding = 0;
    //moduleTileDescriptorSetLayoutBindingInfos[0].flags = vkom::DescriptorFlags::StorageTexture;
    //moduleTileDescriptorSetLayoutBindingInfos[0].count = 1;
    //moduleTileDescriptorSetLayoutBindingInfos[0].stages = vkom::ShaderStageFlags::Compute;

    //vkom::DescriptorSetLayoutInfo moduleTileDescriptorSetLayoutInfo = {};
    //moduleTileDescriptorSetLayoutInfo.bindingCount = 1;
    //moduleTileDescriptorSetLayoutInfo.bindings = &moduleTileDescriptorSetLayoutBindingInfos[0];

    //vkom::IDescriptorSetLayout* moduleTileDescriptorSetLayout;
    //assert(gpuContext.device->createDescriptorSetLayout(&moduleTileDescriptorSetLayoutInfo, &moduleTileDescriptorSetLayout) == vkom::Result::Success);

    /* TODO: more accurate count of descriptor types */
    /*
    vkom::DescriptorPoolDescriptorInfo moduleDescriptorPoolDescriptorInfos[2] = {};
    moduleDescriptorPoolDescriptorInfos[0].flags = vkom::DescriptorFlags::UniformBuffer;
    moduleDescriptorPoolDescriptorInfos[0].count = 256;
    moduleDescriptorPoolDescriptorInfos[1].flags = vkom::DescriptorFlags::StorageTexture;
    moduleDescriptorPoolDescriptorInfos[1].count = 256;
    */

    /* TODO: more accurate count of descriptor sets */
    /*
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
    */

    vkom::AdapterLimits adapterLimits;
    gpuContext.adapter->queryLimits(&adapterLimits);

    vkom::PushConstantRange globalPipelineLayoutPushConstantRange = {};
    globalPipelineLayoutPushConstantRange.offset = 0;
    globalPipelineLayoutPushConstantRange.size = adapterLimits.maxPushConstantsSize;
    globalPipelineLayoutPushConstantRange.stages = vkom::ShaderStageFlags::Compute;

    vkom::PipelineLayoutInfo globalPipelineLayoutInfo = {};
    globalPipelineLayoutInfo.pushConstantRangeCount = 1;
    globalPipelineLayoutInfo.pushConstantRanges = &globalPipelineLayoutPushConstantRange;

    vkom::IPipelineLayout* globalPipelineLayout;
    assert(gpuContext.device->createPipelineLayout(&globalPipelineLayoutInfo, &globalPipelineLayout) == vkom::Result::Success);

    const char* hlslTestModuleSource =
R"(
#define AUDIO_BUFFER_CONFIG_FLAG_VALUE_WRAPAROUND 0x0001
#define AUDIO_BUFFER_CONFIG_FLAG_INDEX_WRAPAROUND 0x0002
#define AUDIO_BUFFER_CONFIG_FLAG_VALUE_REINTERPRET 0x0004
#define AUDIO_BUFFER_CONFIG_FLAG_FORMAT_INT 0x0008
#define AUDIO_BUFFER_CONFIG_FLAG_INDEX_REVERSED 0x0010
#define AUDIO_BUFFER_CONFIG_FLAG_VALUE_NOT_NORMALIZED_TO_BOUNDS 0x0020

#define AUDIO_BUFFER_CONFIG_FLAG_ACCESS_READ 0x4000
#define AUDIO_BUFFER_CONFIG_FLAG_ACCESS_WRITE 0x8000

struct AudioBufferDescriptor {
    uint64_t address;
    uint count;

    uint config;

    float lowerBound;
    float upperBound;
};

struct PushConstants {
    uint dispatchWidth;
    uint sampleRate;
    AudioBufferDescriptor buffers[5];
};

[[vk::push_constant]]
PushConstants pushConstants;

float rangeMap(float t, float inLower, float inUpper, float outLower, float outUpper) {
    float inRange = inUpper - inLower;
    float outRange = outUpper - outLower;

    float normalized = (t - inLower) / inRange;
    return normalized * outRange + outLower;
}

bool read(AudioBufferDescriptor descriptor, uint64_t index, out float value) {
    value = 0;
    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_ACCESS_READ) == 0) {
        return false;
    }

    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_INDEX_WRAPAROUND) != 0) {
        index %= descriptor.count;
    }

    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_INDEX_REVERSED) != 0) {
        index = descriptor.count - index;
    }

    if (descriptor.lowerBound == descriptor.upperBound) {
        value = descriptor.lowerBound;
        return (index < descriptor.count);
    }

    if (index >= descriptor.count || descriptor.address == 0) {
        return false;
    }

    float raw;
    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_FORMAT_INT) != 0 && (descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_VALUE_REINTERPRET) == 0) {
        raw = float(vk::RawBufferLoad<int>(descriptor.address + index * sizeof(int)));
    } else {
        raw = vk::RawBufferLoad<float>(descriptor.address + index * sizeof(float));
    }

    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_VALUE_NOT_NORMALIZED_TO_BOUNDS) != 0) {
        value = rangeMap(raw, -1.0, 1.0, descriptor.lowerBound, descriptor.upperBound);
    } else {
        value = raw;
    }

    return true;
}

bool write(AudioBufferDescriptor descriptor, uint64_t index, float value) {
    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_ACCESS_WRITE) == 0) {
        return false;
    }

    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_INDEX_WRAPAROUND) != 0) {
        index %= descriptor.count;
    }

    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_INDEX_REVERSED) != 0) {
        index = descriptor.count - index;
    }

    if (index >= descriptor.count || descriptor.address == 0) {
        return false;
    }

    float normalized;
    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_VALUE_NOT_NORMALIZED_TO_BOUNDS) != 0) {
        normalized = value;
    } else {
        normalized = rangeMap(value, descriptor.lowerBound, descriptor.upperBound, -1.0, 1.0);
    }

    if ((descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_FORMAT_INT) != 0 && (descriptor.config & AUDIO_BUFFER_CONFIG_FLAG_VALUE_REINTERPRET) == 0) {
        vk::RawBufferStore<int>(descriptor.address + index * sizeof(int), int(normalized));
    } else {
        vk::RawBufferStore<float>(descriptor.address + index * sizeof(float), normalized);
    }

    return true;
}

[numthreads(32, 32, 1)]
void module(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint3 threadCoordinateID : SV_DispatchThreadID) {
    const uint3 groupDimensions = uint3(32, 32, 1);
    const uint3 dispatchDimensions = uint3(pushConstants.dispatchWidth, 1, 1);

    uint localID = groupThreadID.x + groupDimensions.x * (groupThreadID.y + groupDimensions.y * (groupThreadID.z + groupDimensions.z * (groupID.x + dispatchDimensions.x * (groupID.y + dispatchDimensions.y * groupID.z))));
    uint64_t id = localID;

    write(pushConstants.buffers[0], id, float(id % 32) / 16.0 - 1.0);
}
)";

    std::string klatrKlatrSource = R"(
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference : require

layout(buffer_reference) buffer KlatrAudioBufferFloat { float value; };
layout(buffer_reference) buffer KlatrAudioBufferInt { int value; };

#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_WRAPAROUND 0x0001
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_REVERSE 0x0002
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_MIRROR 0x0004

#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_WRAPAROUND 0x0010
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_NON_NORMALIZED 0x0020
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_FORCE_REINTERPRET 0x0040

#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_FORMAT_INT 0x0100

#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_ACCESS_READ 0x4000
#define KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_ACCESS_WRITE 0x8000

struct KlatrAudioBufferDescriptor {
    uint64_t address;
    uint count;
    uint config;

    float lowerBound;
    float upperBound;
};

struct KlatrPushConstants {
    uint sampleRate;
    uint reserved;
    uint64_t dispatchSampleIndex;
    KlatrAudioBufferDescriptor buffers[4];
};

layout(push_constant, std430) uniform _KLATR_PUSH_CONSTANTS {
    KlatrPushConstants _klatrPushConstants;
};

uint64_t abs64(int64_t v) {
    return (v < 0) ? uint64_t(-v) : uint64_t(v);
}

KlatrPushConstants klatrPushConstants() {
    return _klatrPushConstants;
}

bool klatrReadBufferFloat(KlatrAudioBufferDescriptor descriptor, uint64_t index, out float value, float defaultValue) {
    value = defaultValue;
    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_ACCESS_READ) == 0) {
        return false;
    }

    bool constant = (descriptor.lowerBound == descriptor.upperBound);
    if (constant) {
        value = descriptor.lowerBound;
    }

    if (descriptor.count == 0) {
        return false;
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_WRAPAROUND) != 0) {
        if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_MIRROR) != 0) {
            index = ((descriptor.count << 1) - abs64(((int64_t(index % ((descriptor.count << 1) - 1)) - descriptor.count) << 1) + 1) - 1) >> 1;
        } else {
            index = index % descriptor.count;
        }
    } else {
        if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_MIRROR) != 0) {
            if (index >= (descriptor.count << 1)) {
                return false;
            }

            index = ((descriptor.count << 1) - abs64(((int64_t(index) - descriptor.count) << 1) + 1) - 1) >> 1;
        } else if (index >= descriptor.count) {
            return false;
        }
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_REVERSE) != 0) {
        index = descriptor.count - index;
    }

    if (index >= descriptor.count) {
        return false;
    }

    if (constant) {
        return true;
    }

    if (descriptor.address == 0) {
        return false;
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_FORMAT_INT) != 0 && (descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_FORCE_REINTERPRET) == 0) {
        value = float(KlatrAudioBufferInt(descriptor.address + index * 4).value);
    } else {
        value = KlatrAudioBufferFloat(descriptor.address + index * 4).value;
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_NON_NORMALIZED) == 0) {
        value = (value + 1.0) / 2.0 * (descriptor.upperBound - descriptor.lowerBound) + descriptor.lowerBound;
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_WRAPAROUND) != 0) {
        value = mod(value - descriptor.lowerBound, descriptor.upperBound - descriptor.lowerBound) + descriptor.lowerBound;
    }

    return true;
}

bool klatrWriteBufferFloat(KlatrAudioBufferDescriptor descriptor, uint64_t index, float value) {
    if (descriptor.address == 0 || descriptor.count == 0 || (descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_ACCESS_WRITE) == 0) {
        return false;
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_WRAPAROUND) != 0) {
        if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_MIRROR) != 0) {
            index = ((descriptor.count << 1) - abs64(((int64_t(index % ((descriptor.count << 1) - 1)) - descriptor.count) << 1) + 1) - 1) >> 1;
        } else {
            index = index % descriptor.count;
        }
    } else {
        if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_MIRROR) != 0) {
            if (index >= (descriptor.count << 1)) {
                return false;
            }

            index = ((descriptor.count << 1) - abs64(((int64_t(index) - descriptor.count) << 1) + 1) - 1) >> 1;
        } else if (index >= descriptor.count) {
            return false;
        }
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_INDEX_REVERSE) != 0) {
        index = descriptor.count - index;
    }

    if (index >= descriptor.count) {
        return false;
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_NON_NORMALIZED) == 0) {
        value = (value - descriptor.lowerBound) / (descriptor.upperBound - descriptor.lowerBound) * 2.0 - 1.0;
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_WRAPAROUND) != 0) {
        value = mod(value - descriptor.lowerBound, descriptor.upperBound - descriptor.lowerBound) + descriptor.lowerBound;
    }

    if ((descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_FORMAT_INT) != 0 && (descriptor.config & KLATR_AUDIO_BUFFER_DESCRIPTOR_CONFIG_FLAG_VALUE_FORCE_REINTERPRET) == 0) {
        KlatrAudioBufferInt(descriptor.address + index * 4).value = int(value);
    } else {
        KlatrAudioBufferFloat(descriptor.address + index * 4).value = value;
    }

    return true;
}

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

bool module(uint id);

void main() {
    uint id = gl_LocalInvocationID.x + gl_WorkGroupSize.x * (gl_LocalInvocationID.y + gl_WorkGroupSize.y * (gl_LocalInvocationID.z + gl_WorkGroupSize.z * (gl_WorkGroupID.x + gl_NumWorkGroups.x * (gl_WorkGroupID.y + gl_NumWorkGroups.y * gl_WorkGroupID.z))));
    
    if (!module(id)) {
        
    }
}
)";

    const char* testModuleSource = R"(
#include <klatr/klatr.hlsl>

bool module(uint id) {
    float value;
    klatrReadBufferFloat(klatrPushConstants().buffers[1], id + klatrPushConstants().dispatchSampleIndex, value, 0.0);
    klatrWriteBufferFloat(klatrPushConstants().buffers[0], id, value);

    return true;
}
)";

    std::unordered_map<std::string, std::string> systemIncludeFiles = {};
    systemIncludeFiles["klatr/klatr.hlsl"] = klatrKlatrSource;

    std::unordered_map<std::string, std::string> localIncludeFiles = {};

    vkom::IComputePipeline* testModulePipeline = compileComputePipeline(gpuContext.device, globalPipelineLayout, testModuleSource, std::strlen(testModuleSource), systemIncludeFiles, localIncludeFiles);
    assert(testModulePipeline != nullptr);

    //vkom::PushConstantRange modulePipelineLayoutPushConstantRanges[1] = {};
    //modulePipelineLayoutPushConstantRanges[0].offset = 0;
    //modulePipelineLayoutPushConstantRanges[0].size = sizeof(PushConstantAudioBufferDescriptor);
    //modulePipelineLayoutPushConstantRanges[0].stages = vkom::ShaderStageFlags::Compute;

    //vkom::IDescriptorSetLayout* modulePipelineLayoutDescriptorSetLayouts[2] = {};
    //modulePipelineLayoutDescriptorSetLayouts[0] = moduleCommonUniformsDescriptorSetLayout;
    //modulePipelineLayoutDescriptorSetLayouts[1] = moduleTileDescriptorSetLayout;

    //vkom::PipelineLayoutInfo modulePipelineLayoutInfo = {};
    //modulePipelineLayoutInfo.descriptorSetLayoutCount = 2;
    //modulePipelineLayoutInfo.descriptorSetLayouts = &modulePipelineLayoutDescriptorSetLayouts[0];
    //modulePipelineLayoutInfo.pushConstantRangeCount = 1;
    //modulePipelineLayoutInfo.pushConstantRanges = &modulePipelineLayoutPushConstantRanges[0];

    //vkom::IPipelineLayout* modulePipelineLayout;
    //assert(gpuContext.device->createPipelineLayout(&modulePipelineLayoutInfo, &modulePipelineLayout) == vkom::Result::Success);

    //ModuleInfo defaultModule = {};
    //vkom::IComputePipeline* defaultModulePipeline = loadModuleAndPipeline(defaultModule, gpuContext.device, modulePipelineLayout, "module.hlsl.spv");
    //assert(defaultModulePipeline != nullptr);

    //vkom::TextureInfo gpuModuleTileTextureInfo = {};
    //gpuModuleTileTextureInfo.format = vkom::Format::RGBA8UnsignedNorm;
    //gpuModuleTileTextureInfo.samplesPerTexel = 1;
    //gpuModuleTileTextureInfo.usage = vkom::TextureUsageFlags::TransferDestination | vkom::TextureUsageFlags::TransferSource | vkom::TextureUsageFlags::Storage;
    //gpuModuleTileTextureInfo.dimensions.extent.width = 512;
    //gpuModuleTileTextureInfo.dimensions.extent.height = 512;
    //gpuModuleTileTextureInfo.dimensions.subresource.layers = 1;
    //gpuModuleTileTextureInfo.dimensions.subresource.mips = 1;
    //gpuModuleTileTextureInfo.location = vkom::MemoryLocationFlags::GPU;

    //vkom::ITexture* gpuModuleTileTexture;
    //assert(gpuModuleTileTextureHeap->createTexture(&gpuModuleTileTextureInfo, &gpuModuleTileTexture) == vkom::Result::Success);

    //vkom::TextureViewInfo gpuModuleTileTextureViewInfo = {};
    //gpuModuleTileTextureViewInfo.format = gpuModuleTileTextureInfo.format;
    //gpuModuleTileTextureViewInfo.type = vkom::TextureViewType::D2;
    //gpuModuleTileTextureViewInfo.aspectFlags = vkom::TextureAspectFlags::Color;
    //gpuModuleTileTextureViewInfo.subresourceDimensions.layers = 1;
    //gpuModuleTileTextureViewInfo.subresourceDimensions.mips = 1;
    //gpuModuleTileTextureViewInfo.subresourcePosition.layer = 0;
    //gpuModuleTileTextureViewInfo.subresourcePosition.mip = 0;

    //vkom::ITextureView* gpuModuleTileTextureView;
    //assert(gpuModuleTileTexture->createView(&gpuModuleTileTextureViewInfo, &gpuModuleTileTextureView) == vkom::Result::Success);

    vkom::IFence* audioComputeBatchFinishedFence;
    assert(gpuContext.device->acquireFence(false, &audioComputeBatchFinishedFence) == vkom::Result::Success);

    //vkom::IFence* presentBatchFinishedFence;
    //assert(gpuContext.device->acquireFence(false, &presentBatchFinishedFence) == vkom::Result::Success);

    //vkom::IPresentFence* mainSwapchainPresentationFinishedFence = nullptr;

    //vkom::ISemaphore* mainBackbufferAcquisitionSemaphore;
    //assert(gpuContext.device->acquireSemaphore(false, &mainBackbufferAcquisitionSemaphore) == vkom::Result::Success);

    //vkom::ISemaphore* mainBackbufferRenderFinishedSemaphore;
    //assert(gpuContext.device->acquireSemaphore(false, &mainBackbufferRenderFinishedSemaphore) == vkom::Result::Success);

    uint32_t sampleRate = 48000;
    uint32_t channelCount = 1;
    uint32_t frameCount = sampleRate * 10;
    uint32_t sampleCount = frameCount * channelCount;

    vkom::BufferInfo cpuEndpointBufferInfo = {};
    cpuEndpointBufferInfo.size = sizeof(float) * sampleCount;
    cpuEndpointBufferInfo.usage = vkom::BufferUsageFlags::TransferDestination | vkom::BufferUsageFlags::ShaderDeviceAddress;
    cpuEndpointBufferInfo.location = vkom::MemoryLocationFlags::CPU;

    vkom::IBuffer* cpuEndpointBuffer;
    assert(cpuEndpointBufferHeap->createBuffer(&cpuEndpointBufferInfo, &cpuEndpointBuffer) == vkom::Result::Success);

    vkom::IDeviceAddressBuffer* cpuEndpointBufferDA = cpuEndpointBuffer->queryInterface<vkom::IDeviceAddressBuffer>();
    assert(cpuEndpointBufferDA != nullptr);

    //PlaybackBuffer playbackBuffer = {};

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

    ma_decoder_config maDecoderConfig = ma_decoder_config_init(ma_format_s32, channelCount, sampleRate);

    uint64_t maDecodedFrameCount;
    void* maDecodedFrames;

    assert(ma_decode_file("in.wav", &maDecoderConfig, &maDecodedFrameCount, &maDecodedFrames) == MA_SUCCESS);

    vkom::BufferInfo cpuInputBufferInfo = {};
    cpuInputBufferInfo.size = sizeof(float) * channelCount * maDecodedFrameCount;
    cpuInputBufferInfo.usage = vkom::BufferUsageFlags::TransferDestination | vkom::BufferUsageFlags::ShaderDeviceAddress;
    cpuInputBufferInfo.location = vkom::MemoryLocationFlags::CPU;

    vkom::IBuffer* cpuInputBuffer;
    assert(cpuEndpointBufferHeap->createBuffer(&cpuInputBufferInfo, &cpuInputBuffer) == vkom::Result::Success);

    vkom::IDeviceAddressBuffer* cpuInputBufferDA = cpuInputBuffer->queryInterface<vkom::IDeviceAddressBuffer>();
    assert(cpuInputBufferDA != nullptr);

    void* cpuInputBufferMapped = cpuInputBuffer->map();
    std::memcpy(cpuInputBufferMapped, maDecodedFrames, maDecodedFrameCount * channelCount * sizeof(float));
    cpuInputBuffer->unmap();

    ma_free(maDecodedFrames, nullptr);

    //ma_device_config maPlaybackDeviceConfig = {};
    //maPlaybackDeviceConfig.deviceType = ma_device_type_playback;
    //maPlaybackDeviceConfig.sampleRate = 44100;
    //maPlaybackDeviceConfig.dataCallback = [](ma_device* device, void* output, void const* input, ma_uint32 frameCount) {
    //    assert(device->playback.format == ma_format_f32);

    //    PlaybackBuffer* playbackBuffer = reinterpret_cast<PlaybackBuffer*>(device->pUserData);
    //    if (playbackBuffer == nullptr) {
    //        return;
    //    }

    //    size_t count = frameCount * device->playback.channels;
    //    //ma_log_postf(device->pContext->pLog, MA_LOG_LEVEL_INFO, "%zu samples requested, %zu samples received\n", count, playbackBuffer->consume(count, reinterpret_cast<float*>(output), false));
    //};

    //maPlaybackDeviceConfig.pUserData = &playbackBuffer;
    //maPlaybackDeviceConfig.playback.format = ma_format_f32;
    //maPlaybackDeviceConfig.playback.channels = 1;

    //ma_device maPlaybackDevice = {};
    //assert(ma_device_init(&maContext, &maPlaybackDeviceConfig, &maPlaybackDevice) == MA_SUCCESS);
    //assert(ma_device_start(&maPlaybackDevice) == MA_SUCCESS);

    if (renderdoc != nullptr) {
        renderdoc->SetCaptureFilePathTemplate("klatr_rdoc");
        renderdoc->StartFrameCapture(nullptr, nullptr);
    }

    vkom::ICommandEncoder* encoder;
    assert(gpuContext.audioComputeQueue->acquireCommandEncoder(&encoder) == vkom::Result::Success);

    vkom::ComputePassDescriptor cp = {};

    vkom::IComputePass* pass = encoder->beginComputePass(&cp);
    assert(pass != nullptr);

    pass->bindPipeline(testModulePipeline);

    PushConstants pushConstants = {};
    pushConstants.sampleRate = sampleRate;
    pushConstants.dispatchSampleIndex = 0;
    pushConstants.buffers[0].gpuAddress = cpuEndpointBufferDA->deviceAddress();
    pushConstants.buffers[0].count = sampleCount;
    pushConstants.buffers[0].config = AudioBufferConfigFlags::AccessWrite | AudioBufferConfigFlags::IndexMirror;
    pushConstants.buffers[0].lowerBound = -1.0f;
    pushConstants.buffers[0].upperBound = 1.0f;
    pushConstants.buffers[1].gpuAddress = cpuInputBufferDA->deviceAddress();
    pushConstants.buffers[1].count = maDecodedFrameCount;
    pushConstants.buffers[1].config = AudioBufferConfigFlags::AccessRead | AudioBufferConfigFlags::FormatInt | AudioBufferConfigFlags::IndexWraparound;
    pushConstants.buffers[1].lowerBound = 1.0 / std::numeric_limits<int32_t>::min();
    pushConstants.buffers[1].upperBound = 1.0 / std::numeric_limits<int32_t>::max();

    pass->pushConstants(globalPipelineLayout, vkom::ShaderStageFlags::Compute, 0, sizeof(PushConstants), &pushConstants);
    pass->dispatch((sampleCount + 1023) / 1024, 1, 1);
    pass->end();

    vkom::ICommandBatch* batch;
    assert(encoder->batch(&batch) == vkom::Result::Success);

    vkom::CommandBatchSubmitInfo submitInfo = {};
    submitInfo.signalFence = audioComputeBatchFinishedFence;

    assert(batch->submit(&submitInfo) == vkom::Result::Success);

    audioComputeBatchFinishedFence->wait();

    if (renderdoc != nullptr) {
        //renderdoc->TriggerCapture();
        if (renderdoc->EndFrameCapture(nullptr, nullptr) != 1) {
            std::printf("Capture failed\n");
        }
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key >= SDLK_A && event.key.key <= SDLK_Z) {
                        ma_encoder_config maEncoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, channelCount, sampleRate);

                        char filename[10] = "out_X.wav";
                        filename[4] = event.key.key - SDLK_A + 'a';

                        ma_encoder maEncoder = {};
                        assert(ma_encoder_init_file(filename, &maEncoderConfig, &maEncoder) == MA_SUCCESS);

                        void* cpuEndpointBufferMapped = cpuEndpointBuffer->map();
                        assert(cpuEndpointBufferMapped != nullptr);
                        assert(ma_encoder_write_pcm_frames(&maEncoder, cpuEndpointBufferMapped, frameCount, nullptr) == MA_SUCCESS);
                        cpuEndpointBuffer->unmap();

                        ma_encoder_uninit(&maEncoder);
                    }
                    break;
                default:
                    break;
            }
        }

        //if (presentInProgress) {
        //    if (mainSwapchainPresentationFinishedFence != nullptr && mainSwapchainPresentationFinishedFence->status()) {
        //        mainSwapchainPresentationFinishedFence->release();
        //        mainSwapchainPresentationFinishedFence = nullptr;

        //        presentInProgress = false;
        //    }
        //}

        //vkom::IBackbuffer* mainBackbuffer = nullptr;
        //if (!presentInProgress) {
        //    vkom::SemaphorePoint mainBackbufferAcquisitionSignal = {};
        //    mainBackbufferAcquisitionSignal.semaphore = mainBackbufferAcquisitionSemaphore;

        //    uint32_t mainBackbufferIndex;
        //    vkom::Result mainBackbufferAcquisitionResult = mainSwapchain->acquireNextIndex(&mainBackbufferAcquisitionSignal, nullptr, &mainBackbufferIndex);
        //    /* TODO: handle resize */
        //    assert(mainBackbufferAcquisitionResult == vkom::Result::Success);

        //    mainBackbuffer = mainSwapchain->enumerateBackbuffers(mainBackbufferIndex);
        //    assert(mainBackbuffer != nullptr);
        //}

        //if (audioComputeBatchFinishedFence->status()) {
        //    assert(audioComputeBatchFinishedFence->reset() == vkom::Result::Success);

        //    if (audioComputeBatch != nullptr) {
        //        audioComputeBatch->discard();
        //        audioComputeEncoder->release();

        //        audioComputeBatch = nullptr;
        //        audioComputeEncoder = nullptr;
        //    }

        //    audioComputeBatchInProgress = false;

        //    float* cpuEndpointBufferMapped = reinterpret_cast<float*>(cpuEndpointBuffer->map());
        //    assert(playbackBuffer.produce(INTERNAL_AUDIO_BUFFER_FRAME_COUNT * maPlaybackDevice.playback.channels, cpuEndpointBufferMapped, true));
        //    cpuEndpointBuffer->unmap();
        //}

        //if (!audioComputeBatchInProgress) {
        //    assert(gpuContext.audioComputeQueue->acquireCommandEncoder(&audioComputeEncoder) == vkom::Result::Success);

        //    assert(pass.prepare());
        //    assert(pass.bindResourceBySlot(0, cpuEndpointBufferDA->deviceAddress(), INTERNAL_AUDIO_BUFFER_FRAME_COUNT));
        //    assert(pass.execute(audioComputeEncoder, INTERNAL_AUDIO_BUFFER_FRAME_COUNT / 32 / 32, 1, 1, audioComputeBatchFinishedFence));

        //    assert(audioComputeEncoder->batch(&audioComputeBatch) == vkom::Result::Success);

        //    vkom::CommandBatchSubmitInfo submitInfo = {};
        //    submitInfo.signalFence = audioComputeBatchFinishedFence;

        //    assert(audioComputeBatch->submit(&submitInfo) == vkom::Result::Success);

        //    /*
        //    vkom::IResourceView* cpuUniformBufferResourceView = cpuUniformBufferView->queryInterface<vkom::IResourceView>();

        //    vkom::DescriptorWrite moduleCommonUniformsDescriptorSetWrite = {};
        //    moduleCommonUniformsDescriptorSetWrite.binding = 0;
        //    moduleCommonUniformsDescriptorSetWrite.element = 0;
        //    moduleCommonUniformsDescriptorSetWrite.count = 1;
        //    moduleCommonUniformsDescriptorSetWrite.views = &cpuUniformBufferResourceView;

        //    moduleCommonUniformsDescriptorSet->write(1, &moduleCommonUniformsDescriptorSetWrite);

        //    vkom::DescriptorTextureInfo gpuModuleTileDescriptorTextureInfo = {};
        //    gpuModuleTileDescriptorTextureInfo.layout = vkom::TextureLayout::General;

        //    vkom::IResourceView* gpuModuleTileTextureResourceView = gpuModuleTileTextureView->queryInterface<vkom::IResourceView>();

        //    vkom::DescriptorWrite moduleTileDescriptorSetWrite = {};
        //    moduleTileDescriptorSetWrite.binding = 0;
        //    moduleTileDescriptorSetWrite.element = 0;
        //    moduleTileDescriptorSetWrite.count = 1;
        //    moduleTileDescriptorSetWrite.textureInfos = &gpuModuleTileDescriptorTextureInfo;
        //    moduleTileDescriptorSetWrite.views = &gpuModuleTileTextureResourceView;

        //    moduleTileDescriptorSet->write(1, &moduleTileDescriptorSetWrite);

        //    assert(gpuContext.audioComputeQueue->acquireCommandEncoder(&audioComputeEncoder) == vkom::Result::Success);

        //    vkom::ITransferDestinationBuffer* cpuEndpointBufferTD = cpuEndpointBuffer->queryInterface<vkom::ITransferDestinationBuffer>();
        //    assert(cpuEndpointBufferTD != nullptr);

        //    vkom::BufferFill cpuEndpointBufferFill = {};
        //    cpuEndpointBufferFill.dstOffset = 0;
        //    cpuEndpointBufferFill.size = cpuEndpointBufferInfo.size;
        //    cpuEndpointBufferFill.word = 0;

        //    audioComputeEncoder->fillBuffer(cpuEndpointBufferTD, &cpuEndpointBufferFill);

        //    vkom::TextureTransition moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute = {};
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.general.srcStage = vkom::PipelineStageFlags::Transfer;
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.general.dstStage = vkom::PipelineStageFlags::ComputeShader;
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.transfer.oldFamily = gpuContext.presentQueue->family();
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.transfer.newFamily = gpuContext.audioComputeQueue->family();
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.oldLayout = vkom::TextureLayout::Undefined;
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.newLayout = vkom::TextureLayout::General;
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.aspectFlags = vkom::TextureAspectFlags::Color;
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.subresourcePosition.layer = 0;
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.subresourcePosition.mip = 0;
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.subresourceDimensions.layers = 1;
        //    moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute.subresourceDimensions.mips = 1;

        //    audioComputeEncoder->transitionTexture(gpuModuleTileTexture, &moduleTileTransitionToGeneralAndAcquireOwnershipFromPresentToCompute);

        //    vkom::ColorTextureClear moduleTileClear = {};
        //    moduleTileClear.layout = vkom::TextureLayout::General;
        //    moduleTileClear.color[0] = 0.02f;
        //    moduleTileClear.color[1] = 0.02f;
        //    moduleTileClear.color[2] = 0.02f;
        //    moduleTileClear.color[3] = 1.0f;
        //    moduleTileClear.subresourceOffset.layer = 0;
        //    moduleTileClear.subresourceOffset.mip = 0;
        //    moduleTileClear.subresourceRange.layers = 1;
        //    moduleTileClear.subresourceRange.mips = 1;

        //    audioComputeEncoder->clearColorTexture(gpuModuleTileTexture->queryInterface<vkom::ITransferDestinationTexture>(), &moduleTileClear);

        //    vkom::ComputePassDescriptor cpDescriptor = {};

        //    vkom::IComputePass* cp = audioComputeEncoder->beginComputePass(&cpDescriptor);
        //    assert(cp != nullptr);

        //    cp->bindPipeline(defaultModulePipeline);
        //    cp->bindDescriptorSet(modulePipelineLayout, 0, moduleCommonUniformsDescriptorSet, 0, nullptr);
        //    cp->bindDescriptorSet(modulePipelineLayout, 1, moduleTileDescriptorSet, 0, nullptr);

        //    Uniforms uniforms = {};
        //    uniforms.globalID = globalID;
        //    uniforms.globalTime = static_cast<float>(globalID) / static_cast<float>(maPlaybackDeviceConfig.sampleRate);
        //    uniforms.dispatchWidth = (INTERNAL_AUDIO_BUFFER_FRAME_COUNT + 1023) / 1024;
        //    uniforms.sampleRate = maPlaybackDeviceConfig.sampleRate;
        //    uniforms.inverseSampleRate = 1.0f / static_cast<float>(uniforms.sampleRate);

        //    globalID += INTERNAL_AUDIO_BUFFER_FRAME_COUNT;

        //    //std::printf("%u\n", globalID);

        //    void* mappedUniformBuffer = cpuUniformBuffer->map();
        //    std::memcpy(mappedUniformBuffer, &uniforms, sizeof(Uniforms));
        //    cpuUniformBuffer->unmap();

        //    PushConstants pushConstants = {};
        //    pushConstants.buffer.address = cpuEndpointBufferDA->deviceAddress();
        //    pushConstants.buffer.count = INTERNAL_AUDIO_BUFFER_FRAME_COUNT;
        //    pushConstants.buffer.bounds[0] = -1.0f;
        //    pushConstants.buffer.bounds[1] = 1.0f;

        //    cp->pushConstants(modulePipelineLayout, vkom::ShaderStageFlags::Compute, 0, sizeof(pushConstants), &pushConstants);
        //    cp->dispatch(uniforms.dispatchWidth, 1, 1);

        //    cp->end();
        //    cp = nullptr;

        //    vkom::TextureTransition moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent = {};
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.general.srcStage = vkom::PipelineStageFlags::ComputeShader;
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.general.dstStage = vkom::PipelineStageFlags::Transfer;
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.transfer.oldFamily = gpuContext.audioComputeQueue->family();
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.transfer.newFamily = gpuContext.presentQueue->family();
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.oldLayout = vkom::TextureLayout::General;
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.newLayout = vkom::TextureLayout::TransferSource;
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.aspectFlags = vkom::TextureAspectFlags::Color;
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.subresourcePosition.layer = 0;
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.subresourcePosition.mip = 0;
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.subresourceDimensions.layers = 1;
        //    moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent.subresourceDimensions.mips = 1;

        //    audioComputeEncoder->transitionTexture(gpuModuleTileTexture, &moduleTileTransitionFromGeneralToTransferSourceAndReleaseOwnershipFromComputeToPresent);

        //    assert(audioComputeEncoder->batch(&audioComputeBatch) == vkom::Result::Success);

        //    vkom::CommandBatchSubmitInfo submitInfo = {};
        //    submitInfo.signalFence = audioComputeBatchFinishedFence;

        //    assert(audioComputeBatch->submit(&submitInfo) == vkom::Result::Success);
        //    */
        //}

        //if (presentBatchFinishedFence->status()) {
        //    presentBatchFinishedFence->reset();

        //    presentBatch->discard();
        //    presentEncoder->release();

        //    presentBatch = nullptr;
        //    presentEncoder = nullptr;
        //}

        //if (mainBackbuffer != nullptr && presentEncoder == nullptr && mainSwapchainPresentationFinishedFence == nullptr) {
        //    assert(gpuContext.presentQueue->acquireCommandEncoder(&presentEncoder) == vkom::Result::Success);

        //    vkom::TextureTransition mainBackbufferTransitionToTransferDestination = {};
        //    mainBackbufferTransitionToTransferDestination.general.srcStage = vkom::PipelineStageFlags::TopOfPipe;
        //    mainBackbufferTransitionToTransferDestination.general.dstStage = vkom::PipelineStageFlags::TopOfPipe;
        //    mainBackbufferTransitionToTransferDestination.transfer.oldFamily = gpuContext.presentQueue->family();
        //    mainBackbufferTransitionToTransferDestination.transfer.newFamily = gpuContext.presentQueue->family();
        //    mainBackbufferTransitionToTransferDestination.oldLayout = vkom::TextureLayout::Undefined;
        //    mainBackbufferTransitionToTransferDestination.newLayout = vkom::TextureLayout::TransferDestination;
        //    mainBackbufferTransitionToTransferDestination.aspectFlags = vkom::TextureAspectFlags::Color;
        //    mainBackbufferTransitionToTransferDestination.subresourcePosition.layer = 0;
        //    mainBackbufferTransitionToTransferDestination.subresourcePosition.mip = 0;
        //    mainBackbufferTransitionToTransferDestination.subresourceDimensions.layers = 1;
        //    mainBackbufferTransitionToTransferDestination.subresourceDimensions.mips = 1;

        //    presentEncoder->transitionTexture(mainBackbuffer, &mainBackbufferTransitionToTransferDestination);

        //    vkom::ColorTextureClear mainBackbufferClear = {};
        //    mainBackbufferClear.layout = vkom::TextureLayout::TransferDestination;
        //    mainBackbufferClear.color[0] = 1.0f;
        //    mainBackbufferClear.color[1] = 0.0f;
        //    mainBackbufferClear.color[2] = 1.0f;
        //    mainBackbufferClear.color[3] = 1.0f;
        //    mainBackbufferClear.subresourceOffset.layer = 0;
        //    mainBackbufferClear.subresourceOffset.mip = 0;
        //    mainBackbufferClear.subresourceRange.layers = 1;
        //    mainBackbufferClear.subresourceRange.mips = 1;

        //    presentEncoder->clearColorTexture(mainBackbuffer->queryInterface<vkom::ITransferDestinationTexture>(), &mainBackbufferClear);

        //    vkom::TextureTransition moduleTileTransitionTransferSourceAndAcquireOwnershipFromComputeToPresent = {};
        //    mainBackbufferTransitionToTransferDestination.general.srcStage = vkom::PipelineStageFlags::ComputeShader;
        //    mainBackbufferTransitionToTransferDestination.general.dstStage = vkom::PipelineStageFlags::Transfer;
        //    mainBackbufferTransitionToTransferDestination.transfer.oldFamily = gpuContext.audioComputeQueue->family();
        //    mainBackbufferTransitionToTransferDestination.transfer.newFamily = gpuContext.presentQueue->family();
        //    mainBackbufferTransitionToTransferDestination.oldLayout = vkom::TextureLayout::Undefined;
        //    mainBackbufferTransitionToTransferDestination.newLayout = vkom::TextureLayout::TransferSource;
        //    mainBackbufferTransitionToTransferDestination.aspectFlags = vkom::TextureAspectFlags::Color;
        //    mainBackbufferTransitionToTransferDestination.subresourcePosition.layer = 0;
        //    mainBackbufferTransitionToTransferDestination.subresourcePosition.mip = 0;
        //    mainBackbufferTransitionToTransferDestination.subresourceDimensions.layers = 1;
        //    mainBackbufferTransitionToTransferDestination.subresourceDimensions.mips = 1;

        //    presentEncoder->transitionTexture(gpuModuleTileTexture, &mainBackbufferTransitionToTransferDestination);

        //    vkom::TextureBlit blitModuleTileToMainBackbuffer = {};
        //    blitModuleTileToMainBackbuffer.srcLayout = vkom::TextureLayout::TransferSource;
        //    blitModuleTileToMainBackbuffer.srcPositions[0].xyz.x = 0;
        //    blitModuleTileToMainBackbuffer.srcPositions[0].xyz.y = 0;
        //    blitModuleTileToMainBackbuffer.srcPositions[0].xyz.z = 0;
        //    blitModuleTileToMainBackbuffer.srcPositions[0].subresource.layer = 0;
        //    blitModuleTileToMainBackbuffer.srcPositions[0].subresource.mip = 0;
        //    blitModuleTileToMainBackbuffer.srcPositions[1].xyz.x = gpuModuleTileTextureInfo.dimensions.extent.width;
        //    blitModuleTileToMainBackbuffer.srcPositions[1].xyz.y = gpuModuleTileTextureInfo.dimensions.extent.height;
        //    blitModuleTileToMainBackbuffer.srcPositions[1].xyz.z = std::max(1u, gpuModuleTileTextureInfo.dimensions.extent.depth);
        //    blitModuleTileToMainBackbuffer.srcPositions[1].subresource.layer = 0;
        //    blitModuleTileToMainBackbuffer.srcPositions[1].subresource.mip = 1;
        //    blitModuleTileToMainBackbuffer.dstLayout = vkom::TextureLayout::TransferDestination;
        //    blitModuleTileToMainBackbuffer.dstPositions[0].xyz.x = 0;
        //    blitModuleTileToMainBackbuffer.dstPositions[0].xyz.y = 0;
        //    blitModuleTileToMainBackbuffer.dstPositions[0].xyz.z = 0;
        //    blitModuleTileToMainBackbuffer.dstPositions[0].subresource.layer = 0;
        //    blitModuleTileToMainBackbuffer.dstPositions[0].subresource.mip = 0;
        //    blitModuleTileToMainBackbuffer.dstPositions[1].xyz.x = mainSurfaceCapabilities.currentExtent.width;
        //    blitModuleTileToMainBackbuffer.dstPositions[1].xyz.y = mainSurfaceCapabilities.currentExtent.height;
        //    blitModuleTileToMainBackbuffer.dstPositions[1].xyz.z = std::max(1u, mainSurfaceCapabilities.currentExtent.depth);
        //    blitModuleTileToMainBackbuffer.dstPositions[1].subresource.layer = 0;
        //    blitModuleTileToMainBackbuffer.dstPositions[1].subresource.mip = 1;
        //    blitModuleTileToMainBackbuffer.aspectFlags = vkom::TextureAspectFlags::Color;
        //    blitModuleTileToMainBackbuffer.filter = vkom::TexelFilter::Nearest;

        //    presentEncoder->blitTexture(mainBackbuffer->queryInterface<vkom::ITransferDestinationTexture>(), gpuModuleTileTexture->queryInterface<vkom::ITransferSourceTexture>(), &blitModuleTileToMainBackbuffer);

        //    vkom::TextureTransition moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute = {};
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.general.srcStage = vkom::PipelineStageFlags::Transfer;
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.general.dstStage = vkom::PipelineStageFlags::ComputeShader;
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.transfer.oldFamily = gpuContext.presentQueue->family();
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.transfer.newFamily = gpuContext.audioComputeQueue->family();
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.oldLayout = vkom::TextureLayout::TransferSource;
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.newLayout = vkom::TextureLayout::General;
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.aspectFlags = vkom::TextureAspectFlags::Color;
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.subresourcePosition.layer = 0;
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.subresourcePosition.mip = 0;
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.subresourceDimensions.layers = 1;
        //    moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute.subresourceDimensions.mips = 1;

        //    presentEncoder->transitionTexture(gpuModuleTileTexture, &moduleTileTransitionFromTransferSourceToGeneralAndReleaseOwnershipFromPresentToCompute);

        //    vkom::TextureTransition mainBackbufferTransitionFromTransferDestinationToPresentSource = {};
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.general.srcStage = vkom::PipelineStageFlags::Transfer;
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.general.dstStage = vkom::PipelineStageFlags::TopOfPipe;
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.transfer.oldFamily = gpuContext.presentQueue->family();
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.transfer.newFamily = gpuContext.presentQueue->family();
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.oldLayout = vkom::TextureLayout::TransferDestination;
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.newLayout = vkom::TextureLayout::PresentSource;
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.aspectFlags = vkom::TextureAspectFlags::Color;
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.subresourcePosition.layer = 0;
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.subresourcePosition.mip = 0;
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.subresourceDimensions.layers = 1;
        //    mainBackbufferTransitionFromTransferDestinationToPresentSource.subresourceDimensions.mips = 1;

        //    presentEncoder->transitionTexture(mainBackbuffer, &mainBackbufferTransitionFromTransferDestinationToPresentSource);

        //    assert(presentEncoder->batch(&presentBatch) == vkom::Result::Success);

        //    vkom::CommandBatchSubmitWaitInfo presentBatchSubmitWaits[1] = {};
        //    presentBatchSubmitWaits[0].point.semaphore = mainBackbufferAcquisitionSemaphore;
        //    presentBatchSubmitWaits[0].stageFlags = vkom::PipelineStageFlags::TopOfPipe;

        //    vkom::CommandBatchSubmitSignalInfo presentBatchSubmitSignals[1] = {};
        //    presentBatchSubmitSignals[0].point.semaphore = mainBackbufferRenderFinishedSemaphore;

        //    vkom::CommandBatchSubmitInfo presentBatchSubmitInfo = {};
        //    presentBatchSubmitInfo.waitCount = 1;
        //    presentBatchSubmitInfo.waits = &presentBatchSubmitWaits[0];
        //    presentBatchSubmitInfo.signalCount = 1;
        //    presentBatchSubmitInfo.signals = &presentBatchSubmitSignals[0];
        //    presentBatchSubmitInfo.signalFence = presentBatchFinishedFence;

        //    assert(presentBatch->submit(&presentBatchSubmitInfo) == vkom::Result::Success);

        //    vkom::PresentInfo presentInfo = {};
        //    presentInfo.waitCount = 1;
        //    presentInfo.waits = &presentBatchSubmitSignals[0].point;

        //    vkom::Result mainSwapchainPresentationResult = mainSwapchain->present(gpuContext.presentQueue, mainBackbuffer, &presentInfo, &mainSwapchainPresentationFinishedFence);
        //    /* TODO: */
        //    assert(mainSwapchainPresentationResult == vkom::Result::Success);

        //    presentInProgress = true;
        //    mainBackbuffer->release();
        //}

        //ModuleInfo newDefaultModule = {};
        //vkom::IComputePipeline* newDefaultModulePipeline = loadModuleAndPipeline(newDefaultModule, gpuContext.device, modulePipelineLayout, "module.hlsl.spv", defaultModule.lastModified);
        //if (newDefaultModulePipeline != nullptr) {
        //    gpuContext.device->waitIdle();
        //    defaultModulePipeline->release();
        //    defaultModule.shader->release();

        //    defaultModulePipeline = newDefaultModulePipeline;
        //    defaultModule = newDefaultModule;
        //}
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
