#include <klatr/audio/coreaudio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

CoreAudioInstance::CoreAudioInstance() {
    AudioObjectPropertyAddress deviceListProp = {};
    deviceListProp.mSelector = kAudioHardwarePropertyDevices;
    deviceListProp.mScope = kAudioObjectPropertyScopeGlobal;
    deviceListProp.mElement = kAudioObjectPropertyElementMain;

    UInt32 deviceBufferSize;
    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &deviceListProp, 0, nullptr, &deviceBufferSize);

    std::vector<AudioDeviceID> devices(deviceBufferSize / sizeof(AudioDeviceID));
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &deviceListProp, 0, nullptr, &deviceBufferSize, reinterpret_cast<void*>(&devices[0]));

    for (AudioDeviceID deviceID : devices) {
        CoreAudioAdapter* adapter;
        try {
            adapter = new CoreAudioAdapter(this, deviceID);
        } catch (std::runtime_error err) {
            for (CoreAudioAdapter* adapter : _adapters) {
                delete adapter;
            }

            throw err;
        }

        _adapters.push_back(adapter);
        adopt(adapter);
    }
}

CoreAudioInstance::~CoreAudioInstance() {
    ParentByVector::disownAll();

    for (CoreAudioAdapter* adapter : _adapters) {
        delete adapter;
    }
}

/* IInstance */
InstanceBackendFlags CoreAudioInstance::backend() const noexcept {
    return InstanceBackendFlags::CoreAudio;
}

IAdapter* CoreAudioInstance::enumerateAdapters(uint32_t id, IID const& filter) const noexcept {
    IChild* child = enumerateChildren(id, filter);
    if (child == nullptr) {
        return nullptr;
    }

    return child->queryInterface<IAdapter>();
}

IAdapter* CoreAudioInstance::defaultAdapter(DeviceFlowFlags flow) const noexcept {
    for (uint32_t id = 0; true; id += 1) {
        IAdapter* adapter = enumerateAdapters(id, IAdapter::iid());
        if (adapter == nullptr) {
            break;
        }

        AdapterInfo adapterInfo;
        adapter->getInfo(&adapterInfo);

        if ((adapterInfo.defaultForFlow & flow) == flow) {
            return adapter;
        }
    }

    return nullptr;
}

/* IInterface */
void* CoreAudioInstance::queryInterface(IID const& iid) noexcept {
    if (iid == IBase::iid()) {
        return static_cast<IBase*>(this);
    } else if (iid == ICollected::iid()) {
        return static_cast<ICollected*>(this);
    } else if (iid == IParent::iid()) {
        return static_cast<IParent*>(this);
    } else if (iid == IAudioElement::iid()) {
        return static_cast<IAudioElement*>(this);
    } else if (iid == IInstance::iid()) {
        return static_cast<IInstance*>(this);
    }

    return nullptr;
}

IInstance* createInstance() noexcept {
    try {
        CoreAudioInstance* instance = new CoreAudioInstance();
        instance->retain();
        return instance->IInterface::queryInterface<IInstance>();
    } catch (std::runtime_error err) {
        return nullptr;
    }
}

}

}

}

#else

namespace klatr {

namespace audio {

namespace coreaudio {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

#endif
