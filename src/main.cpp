#include <klatr/audio/instance.hpp>
#include <klatr/audio/adapter.hpp>

#include <cassert>

int main(int argc, char** argv) {
    klatr::audio::IInstance* instance = klatr::audio::createInstance(klatr::audio::InstanceBackendFlags::Any);
    assert(instance != nullptr);

    klatr::audio::IInputAdapter* inputAdapter = instance->enumerateAdapters<klatr::audio::IInputAdapter>(0);
    assert(inputAdapter != nullptr);

    klatr::audio::AdapterInfo adapterInfo;
    inputAdapter->getInfo(&adapterInfo);

    klatr::audio::DeviceInfo deviceInfo = {};
    deviceInfo.flow = klatr::audio::DeviceFlowFlags::Input;
    deviceInfo.format = klatr::audio::FormatFlags::Float32;
    deviceInfo.sampleRate = adapterInfo.highestTypicalSampleRate;
    deviceInfo.sampleCount = adapterInfo.highestTypicalSampleRate / 100 * 2; /* ~10 ms of audio */

    klatr::audio::IInputDevice* inputDevice = inputAdapter->createDevice(&deviceInfo)->queryInterface<klatr::audio::IInputDevice>();
    assert(inputDevice != nullptr);

    klatr::audio::IOutputAdapter* outputAdapter = instance->enumerateAdapters<klatr::audio::IOutputAdapter>(1);
    assert(outputAdapter != nullptr);

    outputAdapter->getInfo(&adapterInfo);

    deviceInfo = {};
    deviceInfo.flow = klatr::audio::DeviceFlowFlags::Output;
    deviceInfo.format = klatr::audio::FormatFlags::Float32;
    deviceInfo.sampleRate = adapterInfo.highestTypicalSampleRate;
    deviceInfo.sampleCount = adapterInfo.highestTypicalSampleRate / 100 * 2; /* ~10 ms of audio */

    klatr::audio::IOutputDevice* outputDevice = outputAdapter->createDevice(&deviceInfo)->queryInterface<klatr::audio::IOutputDevice>();
    assert(outputDevice != nullptr);

    outputDevice->start();
    inputDevice->start();

    while (true) {
        klatr::audio::IOutputBuffer* outputBuffer = outputDevice->acquireOutputBuffer(480);
        klatr::audio::IInputBuffer* inputBuffer = inputDevice->acquireInputBuffer();
        if (inputBuffer != nullptr && inputBuffer->empty()) {
            inputBuffer->release();
            inputBuffer = nullptr;
        }

        if (outputBuffer != nullptr) {
            if (inputBuffer != nullptr) {
                uint32_t inputAvailableFrames = inputDevice->currentPadding();
                uint32_t outputAvailableFrames = outputBuffer->frameCount() - outputDevice->currentPadding();

                float* output = reinterpret_cast<float*>(outputBuffer->map());
                float* input = reinterpret_cast<float*>(inputBuffer->map());

                uint32_t frames = std::min(inputAvailableFrames, outputAvailableFrames) * 4 * 2;
                std::memcpy(&output[outputDevice->currentPadding() * 2], input, frames);

                outputBuffer->produce(outputBuffer->frameCount());

                inputBuffer->unmap();
                outputBuffer->unmap();
            }

            outputBuffer->release();
        }

        if (inputBuffer != nullptr) {
            inputBuffer->consume();
            inputBuffer->release();
        }
    }

    outputDevice->release();
    inputDevice->release();
    instance->release();
    return 0;
}
