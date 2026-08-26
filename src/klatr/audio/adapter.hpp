#pragma once

#include <kom/kom.hpp>

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

#include <klatr/audio/audio.hpp>
#include <klatr/audio/enums.hpp>
#include <klatr/audio/device.hpp>

namespace klatr {

namespace audio {

struct AdapterInfo {
    kom::UUID uuid;
    DeviceChannelFlags channels;
    DeviceTransportType transport;
    DeviceFlowFlags flow;
    DeviceFlowFlags defaultForFlow;
    FormatFlags formats;
    uint32_t lowestTypicalSampleRate;
    uint32_t highestTypicalSampleRate;
    char name[128];
};

class IAdapter : virtual public IAudioElement, virtual public IParent, virtual public IChild {
public:
    virtual void getInfo(AdapterInfo* info) const noexcept = 0;
    virtual IDevice* createDevice(DeviceInfo const* info) noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("a518eef9-a2cf-40a4-9dae-d1899558bf44");
        return iid;
    }
};

class IOutputAdapter : virtual public IAdapter {
public:
    static inline IID const& iid() noexcept {
        static IID iid = IID("b10815d5-372a-47b4-bf0a-89f0fa62650f");
        return iid;
    }
};

class IInputAdapter : virtual public IAdapter {
public:
    static inline IID const& iid() noexcept {
        static IID iid = IID("952f30e9-a3e2-4655-894e-3854f90cb7fa");
        return iid;
    }
};

}

}
