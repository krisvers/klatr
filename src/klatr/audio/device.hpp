#pragma once

#include <kom/kom.hpp>

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

#include <klatr/audio/audio.hpp>
#include <klatr/audio/enums.hpp>

namespace klatr {

namespace audio {

struct DeviceInfo {
    DeviceFlowFlags flow;
    FormatFlags format;
    uint32_t sampleRate;
    uint32_t sampleCount;
};

class IDevice : virtual public IAudioElement, virtual public ICollected, virtual public IParent, virtual public IChild {
public:
    virtual void getInfo(DeviceInfo* info) const noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("26570820-2ac4-4f68-bc6f-1a7c1f479193");
        return iid;
    }
};

}

}
