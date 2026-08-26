#pragma once

#include <kom/kom.hpp>

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

#include <klatr/audio/audio.hpp>
#include <klatr/audio/enums.hpp>
#include <klatr/audio/buffer.hpp>

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

    virtual bool start(DeviceFlowFlags flow) noexcept = 0;
    virtual bool stop(DeviceFlowFlags flow) noexcept = 0;

    virtual uint32_t currentPadding() const noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("26570820-2ac4-4f68-bc6f-1a7c1f479193");
        return iid;
    }
};

class IOutputDevice : virtual public IDevice {
public:
    virtual IOutputBuffer* acquireOutputBuffer(uint32_t frameCount) noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("5fb70c53-1952-4b88-b4c0-a341dbf2734d");
        return iid;
    }
};

class IInputDevice : virtual public IDevice {
public:
    virtual IInputBuffer* acquireInputBuffer() noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("bbd1f0a4-7776-40ae-a068-a5ac09fc9a56");
        return iid;
    }
};

}

}
