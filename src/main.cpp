#include <klatr/audio/instance.hpp>
#include <klatr/audio/adapter.hpp>

#include <klatr/gpu/context.hpp>

#include <cassert>

int main(int argc, char** argv) {
    klatr::gpu::Context gpuContext = klatr::gpu::Context(true);

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
