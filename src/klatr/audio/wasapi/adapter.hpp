#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/adapter.hpp>

#include <klatr/audio/instance.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

/* NOTE: adapter does not publicly advertise ICollected support, but it is used for cleanup QoL */

class WASAPIAdapter : virtual public IAdapter, virtual public CollectedByHeap, virtual public ParentByVector {
private:
    IInstance* _instance = nullptr;
    IMMDevice* _mmDevice = nullptr;

    AdapterInfo _info = {};

public:
    WASAPIAdapter(IInstance* instance, IMMDevice* mmDevice, DeviceFlowFlags flow);
    ~WASAPIAdapter();

    /* IAdapter */
    void getInfo(AdapterInfo* info) const noexcept override;
    IDevice* createDevice(DeviceInfo const* info) noexcept override;

    /* IParent */
    IParent* parent() const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;
};

}

}

}

#endif
