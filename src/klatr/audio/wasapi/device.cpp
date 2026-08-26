#include <klatr/audio/wasapi/device.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

WASAPIDevice::WASAPIDevice(IAdapter* adapter, IAudioClient* audioClient, DeviceInfo const& info) : _adapter(adapter), _instance(_adapter->parent<IInstance>()), _audioClient(audioClient), _info(info) {
    
}

WASAPIDevice::~WASAPIDevice() {
    _adapter->disown(IInterface::queryInterface<IChild>());

    ParentByVector::disownAll();

    _audioClient->Release();
}

/* IDevice */
void WASAPIDevice::getInfo(DeviceInfo* info) const noexcept {
    std::memcpy(info, &_info, sizeof(DeviceInfo));
}

/* IParent */
IParent* WASAPIDevice::parent() const noexcept {
    return _adapter->queryInterface<IParent>();
}

/* IInterface */
void* WASAPIDevice::queryInterface(IID const& iid) noexcept {
    if (iid == IBase::iid()) {
        return static_cast<IBase*>(this);
    } else if (iid == ICollected::iid()) {
        return static_cast<ICollected*>(this);
    } else if (iid == IParent::iid()) {
        return static_cast<IParent*>(this);
    } else if (iid == IChild::iid()) {
        return static_cast<IChild*>(this);
    } else if (iid == IAudioElement::iid()) {
        return static_cast<IAudioElement*>(this);
    } else if (iid == IDevice::iid()) {
        return static_cast<IDevice*>(this);
    }

    return nullptr;
}

}

}

}

#endif
