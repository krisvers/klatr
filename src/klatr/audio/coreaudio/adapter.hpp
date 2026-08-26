#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/adapter.hpp>
#include <klatr/audio/instance.hpp>

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

/* NOTE: while this implements IOutputAdapter and IInputAdapter, only
    instances that support each flow will advertise their interfaces
*/

class CoreAudioAdapter : virtual public IAdapter, virtual public IOutputAdapter, virtual public IInputAdapter, virtual public ParentByVector {
private:
    IInstance* _instance = nullptr;
    AudioDeviceID _deviceID = {};

    AdapterInfo _info = {};

public:
    CoreAudioAdapter(IInstance* instance, AudioDeviceID deviceID);
    ~CoreAudioAdapter();

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
