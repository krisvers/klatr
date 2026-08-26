#pragma once

#include <klatr/audio/audio.hpp>
#include <klatr/audio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <vector>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

class WASAPIAdapter;

class WASAPIInstance : virtual public IInstance, virtual public CollectedByHeap, virtual public ParentByVector {
private:
    IMMDeviceEnumerator* _mmDeviceEnumerator = nullptr;

    std::vector<WASAPIAdapter*> _adapters = {};

public:
    WASAPIInstance();
    ~WASAPIInstance();

    /* IInstance */
    InstanceBackendFlags backend() const noexcept override;
    IAdapter* enumerateAdapters(uint32_t id, IID const& filter) const noexcept override;
    IAdapter* defaultAdapter(DeviceFlowFlags flow) const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;
};

}

}

}

#endif

namespace klatr {

namespace audio {

namespace wasapi {

IInstance* createInstance() noexcept;

}

}

}
