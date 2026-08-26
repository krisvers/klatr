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

class WASAPIDevice : virtual public IDevice, virtual public CollectedByHeap, virtual public ParentByVector {
private:
    IAdapter* _adapter = nullptr;
    IInstance* _instance = nullptr;
    IAudioClient* _audioClient = nullptr;
    DeviceInfo _info = {};

public:
    WASAPIDevice(IAdapter* adapter, IAudioClient* audioClient, DeviceInfo const& info);
    ~WASAPIDevice();

    /* IDevice */
    void getInfo(DeviceInfo* info) const noexcept override;

    /* IParent */
    IParent* parent() const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;
};

}

}

}

#endif
