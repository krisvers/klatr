#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/adapter.hpp>

#include <klatr/audio/instance.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

/* NOTE: while this implements IOutputAdapter and IInputAdapter, only
    instances that support each flow will advertise their interfaces
*/

class WASAPIAdapter : virtual public IAdapter, virtual public IOutputAdapter, virtual public IInputAdapter, virtual public ParentByVector {
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

    /* IChild */
    IParent* parent() const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;
};

}

}

}

#endif
