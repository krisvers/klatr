#include <klatr/gpu/context.hpp>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <SDL3/SDL.h>

#include <bitset>
#include <fstream>
#include <cassert>

#define INTERNAL_AUDIO_BUFFER_FRAME_COUNT 65536

struct UniformAudioBufferDescriptor {
    float bounds[2]; /* note: if bounds[0] == bounds[1] this is treated as a constant */
    uint32_t count;
};

struct Uniforms {
    uint64_t globalID;
    uint32_t sampleRate;
    UniformAudioBufferDescriptor buffer;
};

struct PushConstantAudioBufferDescriptor {
    uint64_t address;
    float bounds[2]; /* note: if bounds[0] == bounds[1] this is treated as a constant */
    uint32_t count;
};

struct PushConstants {
    PushConstantAudioBufferDescriptor buffer;
};

struct PlaybackBuffer {
    uint32_t sampleCount;
    float* samples;

    uint32_t start;
    uint32_t end;

    uint32_t playedUntil;
    uint64_t totalSamplesPlayed;
};

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

int main(int argc, char** argv) {
    klatr::gpu::Context gpuContext = klatr::gpu::Context(true);
    gpuContext.instance->setLogCallback([](vkom::IInstance* instance, void* userData, vkom::DebugMessageSeverityFlags severity, vkom::DebugMessageTypeFlags types, const char* message) {
        std::printf("[vkom]: %s\n", message);
    }, nullptr);

    assert(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* mainWindow = SDL_CreateWindow("klatr | main", 1200, 800, 0);
    assert(mainWindow != nullptr);

    vkom::IHeap* cpuEndpointBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::StorageBuffer | vkom::BufferUsageFlags::ShaderDeviceAddress, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::CPU, &cpuEndpointBufferHeap) == vkom::Result::Success);

    vkom::IHeap* gpuTransientBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::StorageBuffer | vkom::BufferUsageFlags::ShaderDeviceAddress, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::GPU, &gpuTransientBufferHeap) == vkom::Result::Success);

    vkom::IHeap* cpuUniformBufferHeap;
    assert(gpuContext.device->createHeap(vkom::BufferUsageFlags::UniformBuffer, vkom::TextureUsageFlags::None, vkom::MemoryLocationFlags::CPU, &cpuUniformBufferHeap) == vkom::Result::Success);

    vkom::DescriptorBindingInfo moduleDescriptorSetLayoutBindingInfos[2] = {};
    moduleDescriptorSetLayoutBindingInfos[0].binding = 0;
    moduleDescriptorSetLayoutBindingInfos[0].flags = vkom::DescriptorFlags::UniformBuffer;
    moduleDescriptorSetLayoutBindingInfos[0].count = 1;
    moduleDescriptorSetLayoutBindingInfos[0].stages = vkom::ShaderStageFlags::Compute;
    moduleDescriptorSetLayoutBindingInfos[1].binding = 1;
    moduleDescriptorSetLayoutBindingInfos[1].flags = vkom::DescriptorFlags::StorageBuffer;
    moduleDescriptorSetLayoutBindingInfos[1].count = 1;
    moduleDescriptorSetLayoutBindingInfos[1].stages = vkom::ShaderStageFlags::Compute;

    vkom::DescriptorSetLayoutInfo moduleDescriptorSetLayoutInfo = {};
    moduleDescriptorSetLayoutInfo.bindingCount = 2;
    moduleDescriptorSetLayoutInfo.bindings = &moduleDescriptorSetLayoutBindingInfos[0];

    vkom::IDescriptorSetLayout* moduleDescriptorSetLayout;
    assert(gpuContext.device->createDescriptorSetLayout(&moduleDescriptorSetLayoutInfo, &moduleDescriptorSetLayout) == vkom::Result::Success);

    vkom::DescriptorPoolDescriptorInfo moduleDescriptorPoolDescriptorInfos[2] = {};
    moduleDescriptorPoolDescriptorInfos[0].flags = vkom::DescriptorFlags::UniformBuffer;
    moduleDescriptorPoolDescriptorInfos[0].count = 1;
    moduleDescriptorPoolDescriptorInfos[1].flags = vkom::DescriptorFlags::StorageBuffer;
    moduleDescriptorPoolDescriptorInfos[1].count = 1;

    vkom::DescriptorPoolInfo moduleDescriptorPoolInfo = {};
    moduleDescriptorPoolInfo.maxDescriptorSets = 1;
    moduleDescriptorPoolInfo.descriptorCount = 2;
    moduleDescriptorPoolInfo.descriptors = &moduleDescriptorPoolDescriptorInfos[0];

    vkom::IDescriptorPool* moduleDescriptorPool;
    assert(gpuContext.device->createDescriptorPool(&moduleDescriptorPoolInfo, &moduleDescriptorPool) == vkom::Result::Success);

    vkom::IDescriptorSet* moduleDescriptorSet;
    assert(moduleDescriptorPool->allocateDescriptorSets(moduleDescriptorSetLayout, 1, &moduleDescriptorSet) == vkom::Result::Success);

    vkom::PushConstantRange modulePipelineLayoutPushConstantRanges[1] = {};
    modulePipelineLayoutPushConstantRanges[0].offset = 0;
    modulePipelineLayoutPushConstantRanges[0].size = sizeof(PushConstantAudioBufferDescriptor);
    modulePipelineLayoutPushConstantRanges[0].stages = vkom::ShaderStageFlags::Compute;

    vkom::PipelineLayoutInfo modulePipelineLayoutInfo = {};
    modulePipelineLayoutInfo.descriptorSetLayoutCount = 1;
    modulePipelineLayoutInfo.descriptorSetLayouts = &moduleDescriptorSetLayout;
    modulePipelineLayoutInfo.pushConstantRangeCount = 1;
    modulePipelineLayoutInfo.pushConstantRanges = &modulePipelineLayoutPushConstantRanges[0];

    vkom::IPipelineLayout* modulePipelineLayout;
    assert(gpuContext.device->createPipelineLayout(&modulePipelineLayoutInfo, &modulePipelineLayout) == vkom::Result::Success);

    std::vector<uint32_t> moduleShaderSPIRV = loadFile("module.hlsl.spv");
    assert(!moduleShaderSPIRV.empty());

    vkom::ShaderModuleInfo moduleShaderInfo = {};
    moduleShaderInfo.length = moduleShaderSPIRV.size();
    moduleShaderInfo.spirv = &moduleShaderSPIRV[0];

    vkom::IShaderModule* moduleShader;
    assert(gpuContext.device->createShaderModule(&moduleShaderInfo, &moduleShader) == vkom::Result::Success);

    vkom::ComputePipelineInfo modulePipelineInfo = {};
    modulePipelineInfo.shaderInfo.shader = moduleShader;
    modulePipelineInfo.shaderInfo.stage = vkom::ShaderStageFlags::Compute;
    modulePipelineInfo.shaderInfo.entry = "module";

    vkom::IComputePipeline* modulePipeline;
    assert(gpuContext.device->createComputePipeline(&modulePipelineInfo, nullptr, modulePipelineLayout, &modulePipeline) == vkom::Result::Success);

    vkom::IFence* batchFinishedFence;
    assert(gpuContext.device->acquireFence(false, &batchFinishedFence) == vkom::Result::Success);

    vkom::BufferInfo cpuEndpointBufferInfo = {};
    cpuEndpointBufferInfo.size = sizeof(float) * INTERNAL_AUDIO_BUFFER_FRAME_COUNT;
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
    playbackBuffer.sampleCount = 16384;
    playbackBuffer.samples = new float[playbackBuffer.sampleCount];
    playbackBuffer.start = 0;
    playbackBuffer.end = 0;
    playbackBuffer.playedUntil = 0;

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
    maPlaybackDeviceConfig.sampleRate = 48000;
    maPlaybackDeviceConfig.dataCallback = [](ma_device* device, void* output, void const* input, ma_uint32 frameCount) {
        assert(device->playback.format == ma_format_f32);

        PlaybackBuffer* playbackBuffer = reinterpret_cast<PlaybackBuffer*>(device->pUserData);
        if (playbackBuffer == nullptr) {
            return;
        }

        uint32_t playbackStart = playbackBuffer->start;
        uint32_t playbackEnd = playbackBuffer->end;
        uint32_t playbackSampleCount = playbackBuffer->sampleCount;

        uint32_t requestedFrameCount = frameCount;
        uint32_t availableSampleCounts[2] = {};
        if (playbackEnd < playbackStart) {
            availableSampleCounts[0] = playbackSampleCount - playbackStart;
            availableSampleCounts[1] = playbackEnd;
        } else {
            availableSampleCounts[0] = playbackEnd - playbackStart;
            availableSampleCounts[1] = 0;
        }

        uint32_t channels = device->playback.channels;
        uint32_t availableFrameCounts[2] = {};
        availableFrameCounts[0] = availableSampleCounts[0] / channels;
        availableFrameCounts[1] = availableSampleCounts[1] / channels;

        uint32_t totalAvailableFrameCount = availableFrameCounts[0] + availableFrameCounts[1];
        uint32_t totalCopiableFrameCount = std::min(totalAvailableFrameCount, requestedFrameCount);

        uint32_t copiableFrameCounts[2] = {};
        copiableFrameCounts[0] = std::min(availableFrameCounts[0], totalCopiableFrameCount);
        if (totalCopiableFrameCount > availableFrameCounts[0]) {
            copiableFrameCounts[1] = (totalCopiableFrameCount - availableFrameCounts[0]);
        }

        if (copiableFrameCounts[0] == 0) {
            return;
        }

        float* playbackBufferStarts[2] = {};
        playbackBufferStarts[0] = &playbackBuffer->samples[playbackStart];

        float* outputStarts[2] = {};
        outputStarts[0] = reinterpret_cast<float*>(output);
        if (copiableFrameCounts[1] != 0) {
            playbackBufferStarts[1] = &playbackBuffer->samples[copiableFrameCounts[0] * channels];
            outputStarts[1] = &reinterpret_cast<float*>(output)[copiableFrameCounts[0] * channels];
        }

        std::memcpy(outputStarts[0], playbackBufferStarts[0], copiableFrameCounts[0] * channels * sizeof(float));
        playbackBuffer->playedUntil = playbackStart + copiableFrameCounts[0] * channels;

        if (copiableFrameCounts[1] != 0) {
            std::memcpy(outputStarts[1], playbackBufferStarts[1], copiableFrameCounts[1] * channels * sizeof(float));
            playbackBuffer->playedUntil = copiableFrameCounts[1] * channels;
        }

        playbackBuffer->totalSamplesPlayed += totalCopiableFrameCount * channels;
    };

    maPlaybackDeviceConfig.pUserData = &playbackBuffer;
    maPlaybackDeviceConfig.playback.format = ma_format_f32;
    maPlaybackDeviceConfig.playback.channels = 2;

    ma_device maPlaybackDevice = {};
    assert(ma_device_init(&maContext, &maPlaybackDeviceConfig, &maPlaybackDevice) == MA_SUCCESS);
    assert(ma_device_start(&maPlaybackDevice) == MA_SUCCESS);

    uint64_t globalID = 0;
    uint32_t previousPlayedUntil = 0;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                default:
                    break;
            }
        }

        vkom::IResourceView* cpuUniformBufferResourceView = cpuUniformBufferView->queryInterface<vkom::IResourceView>();
        vkom::IResourceView* cpuEndpointBufferResourceView = cpuEndpointBufferView->queryInterface<vkom::IResourceView>();

        vkom::DescriptorWrite moduleDescriptorSetWrites[2] = {};
        moduleDescriptorSetWrites[0].binding = 0;
        moduleDescriptorSetWrites[0].element = 0;
        moduleDescriptorSetWrites[0].count = 1;
        moduleDescriptorSetWrites[0].views = &cpuUniformBufferResourceView;
        moduleDescriptorSetWrites[1].binding = 1;
        moduleDescriptorSetWrites[1].element = 0;
        moduleDescriptorSetWrites[1].count = 1;
        moduleDescriptorSetWrites[1].views = &cpuEndpointBufferResourceView;

        moduleDescriptorSet->write(2, &moduleDescriptorSetWrites[0]);

        vkom::ICommandEncoder* encoder;
        assert(gpuContext.audioComputeQueue->acquireCommandEncoder(&encoder) == vkom::Result::Success);

        vkom::ITransferDestinationBuffer* cpuEndpointBufferTD = cpuEndpointBuffer->queryInterface<vkom::ITransferDestinationBuffer>();
        assert(cpuEndpointBufferTD != nullptr);

        vkom::BufferFill cpuEndpointBufferFill = {};
        cpuEndpointBufferFill.dstOffset = 0;
        cpuEndpointBufferFill.size = cpuEndpointBufferInfo.size;
        cpuEndpointBufferFill.word = 0;

        encoder->fillBuffer(cpuEndpointBufferTD, &cpuEndpointBufferFill);

        vkom::ComputePassDescriptor cpDescriptor = {};

        vkom::IComputePass* cp = encoder->beginComputePass(&cpDescriptor);
        assert(cp != nullptr);

        cp->bindPipeline(modulePipeline);
        cp->bindDescriptorSet(modulePipelineLayout, 0, moduleDescriptorSet, 0, nullptr);

        Uniforms uniforms = {};
        uniforms.globalID = globalID;
        uniforms.sampleRate = maPlaybackDeviceConfig.sampleRate;
        uniforms.buffer.bounds[0] = -1.0f;
        uniforms.buffer.bounds[1] = 1.0f;
        uniforms.buffer.count = INTERNAL_AUDIO_BUFFER_FRAME_COUNT;

        //std::printf("%u\n", globalID);

        void* mappedUniformBuffer = cpuUniformBuffer->map();
        std::memcpy(mappedUniformBuffer, &uniforms, sizeof(Uniforms));
        cpuUniformBuffer->unmap();

        PushConstants pushConstants = {};
        pushConstants.buffer.address = cpuEndpointBufferDA->deviceAddress();
        pushConstants.buffer.bounds[0] = uniforms.buffer.bounds[0];
        pushConstants.buffer.bounds[1] = uniforms.buffer.bounds[1];
        pushConstants.buffer.count = uniforms.buffer.count;

        cp->pushConstants(modulePipelineLayout, vkom::ShaderStageFlags::Compute, 0, sizeof(pushConstants), &pushConstants);
        cp->dispatch(INTERNAL_AUDIO_BUFFER_FRAME_COUNT, 1, 1);

        cp->end();
        cp = nullptr;

        vkom::ICommandBatch* batch;
        assert(encoder->batch(&batch) == vkom::Result::Success);

        vkom::CommandBatchSubmitInfo submitInfo = {};
        submitInfo.signalFence = batchFinishedFence;

        assert(batch->submit(&submitInfo) == vkom::Result::Success);
        assert(batchFinishedFence->wait() == vkom::Result::Success);
        assert(batchFinishedFence->reset() == vkom::Result::Success);

        std::printf("%u %u %u %llu\n", playbackBuffer.start, playbackBuffer.end, playbackBuffer.playedUntil, playbackBuffer.totalSamplesPlayed);

        uint32_t newStart = playbackBuffer.end % playbackBuffer.sampleCount;
        uint32_t producedSampleCount = cpuEndpointBufferInfo.size / sizeof(float);
        uint32_t availableSampleCounts[2] = {};
        availableSampleCounts[0] = playbackBuffer.sampleCount - newStart;
        availableSampleCounts[1] = newStart;

        uint32_t totalCopiableSampleCount = std::min(producedSampleCount, playbackBuffer.sampleCount);

        uint32_t copiableSampleCounts[2] = {};
        copiableSampleCounts[0] = std::min(producedSampleCount, availableSampleCounts[0]);
        if (totalCopiableSampleCount > availableSampleCounts[0]) {
            copiableSampleCounts[1] = totalCopiableSampleCount - availableSampleCounts[0];
        }

        uint32_t newEnd = playbackBuffer.end;
        float* cpuEndpointMapped = reinterpret_cast<float*>(cpuEndpointBuffer->map());
        if (copiableSampleCounts[0] != 0) {
            assert(newStart + copiableSampleCounts[0] <= playbackBuffer.sampleCount);
            assert(copiableSampleCounts[0] <= cpuEndpointBufferInfo.size / sizeof(float));

            std::memcpy(&playbackBuffer.samples[newStart], cpuEndpointMapped, copiableSampleCounts[0] * sizeof(float));
            newEnd = newStart + copiableSampleCounts[0];
        }

        if (copiableSampleCounts[1] != 0) {
            assert(copiableSampleCounts[1] < playbackBuffer.sampleCount);
            assert(copiableSampleCounts[0] + copiableSampleCounts[1] <= cpuEndpointBufferInfo.size / sizeof(float));

            std::memcpy(&playbackBuffer.samples[0], &cpuEndpointMapped[copiableSampleCounts[0]], copiableSampleCounts[1] * sizeof(float));
            newEnd = copiableSampleCounts[1];
        }

        playbackBuffer.start = newStart;
        playbackBuffer.end = newEnd;
        cpuEndpointBuffer->unmap();

        globalID += totalCopiableSampleCount / maPlaybackDevice.playback.channels;

        batch->discard();
        encoder->release();
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
