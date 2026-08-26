#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/device.hpp>
#include <klatr/audio/adapter.hpp>
#include <klatr/audio/instance.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

/* NOTE: while this implements IOutputDevice and IInputDevice, only
    instances that support each flow will advertise their interfaces
*/

class WASAPIDevice : virtual public IDevice, virtual public IOutputDevice, virtual public IInputDevice, virtual public CollectedByHeap, virtual public ParentByVector {
private:
    IAdapter* _adapter = nullptr;
    IInstance* _instance = nullptr;
    IAudioClient* _audioClient = nullptr;
    IAudioRenderClient* _audioRenderClient = nullptr;
    IAudioCaptureClient* _audioCaptureClient = nullptr;
    DeviceInfo _info = {};

public:
    WASAPIDevice(IAdapter* adapter, IAudioClient* audioClient, DeviceInfo const& info);
    ~WASAPIDevice();

    /* IOutputDevice */
    IOutputBuffer* acquireOutputBuffer(uint32_t frameCount) noexcept override;

    /* IInputDevice */
    IInputBuffer* acquireInputBuffer() noexcept override;

    /* IDevice */
    void getInfo(DeviceInfo* info) const noexcept override;

    bool start(DeviceFlowFlags flow) noexcept override;
    bool stop(DeviceFlowFlags flow) noexcept override;

    uint32_t currentPadding() const noexcept override;

    /* IChild */
    IParent* parent() const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;
};

}

}

}

#endif
