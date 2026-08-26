#include <klatr/audio/instance.hpp>
#include <klatr/audio/adapter.hpp>

#include <cassert>

int main(int argc, char** argv) {
    klatr::audio::IInstance* instance = klatr::audio::createInstance(klatr::audio::InstanceBackendFlags::Any);
    assert(instance != nullptr);

    klatr::audio::IOutputAdapter* adapter = instance->enumerateAdapters<klatr::audio::IOutputAdapter>(0);
    assert(adapter != nullptr);

    klatr::audio::AdapterInfo adapterInfo;
    adapter->getInfo(&adapterInfo);

    klatr::audio::DeviceInfo deviceInfo = {};
    deviceInfo.flow = klatr::audio::DeviceFlowFlags::Output;
    deviceInfo.format = klatr::audio::FormatFlags::Float32;
    deviceInfo.sampleRate = adapterInfo.highestTypicalSampleRate;
    deviceInfo.sampleCount = adapterInfo.highestTypicalSampleRate / 100; /* ~10 ms of audio */

    klatr::audio::IDevice* device = adapter->createDevice(&deviceInfo);
    assert(device != nullptr);

    device->release();
    instance->release();
    return 0;
}
