#include <klatr/audio/wasapi/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <stdexcept>

#include <klatr/audio/wasapi/adapter.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

WASAPIInstance::WASAPIInstance() {
    if (!SUCCEEDED(CoInitialize(nullptr))) {
        throw std::runtime_error("CoInitialize failed");
    }

    if (!SUCCEEDED(CoCreateInstance(__uuidof(IMMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&_mmDeviceEnumerator)))) {
        throw std::runtime_error("CoCreateInstance for IMMDeviceEnumerator failed");
    }

    IMMDeviceCollection* mmRenderDeviceCollection;
    if (!SUCCEEDED(_mmDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &mmRenderDeviceCollection))) {
        throw std::runtime_error("IMMDeviceEnumerator::EnumAudioEndpoints failed");
    }

    UINT renderDeviceCount;
    if (!SUCCEEDED(mmRenderDeviceCollection->GetCount(&renderDeviceCount))) {
        mmRenderDeviceCollection->Release();
        throw std::runtime_error("IMMDeviceCollection::GetCount failed");
    }

    for (UINT i = 0; i < renderDeviceCount; i += 1) {
        IMMDevice* mmDevice;
        if (!SUCCEEDED(mmRenderDeviceCollection->Item(i, &mmDevice))) {
            mmRenderDeviceCollection->Release();
            throw std::runtime_error("IMMDeviceCollection::Item failed");
        }

        WASAPIAdapter* adapter;
        try {
            adapter = new WASAPIAdapter(this, mmDevice, DeviceFlowFlags::Output);
        } catch (std::runtime_error err) {
            mmDevice->Release();
            mmRenderDeviceCollection->Release();
            throw err;
        }

        adopt(adapter->IInterface::queryInterface<IChild>());
    }

    mmRenderDeviceCollection->Release();

    IMMDeviceCollection* mmCaptureDeviceCollection;
    if (!SUCCEEDED(_mmDeviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &mmCaptureDeviceCollection))) {
        throw std::runtime_error("IMMDeviceEnumerator::EnumAudioEndpoints failed");
    }

    UINT captureDeviceCount;
    if (!SUCCEEDED(mmCaptureDeviceCollection->GetCount(&captureDeviceCount))) {
        mmCaptureDeviceCollection->Release();
        throw std::runtime_error("IMMDeviceCollection::GetCount failed");
    }

    for (UINT i = 0; i < captureDeviceCount; i += 1) {
        IMMDevice* mmDevice;
        if (!SUCCEEDED(mmCaptureDeviceCollection->Item(i, &mmDevice))) {
            mmCaptureDeviceCollection->Release();
            throw std::runtime_error("IMMDeviceCollection::Item failed");
        }

        WASAPIAdapter* adapter;
        try {
            adapter = new WASAPIAdapter(this, mmDevice, DeviceFlowFlags::Input);
        } catch (std::runtime_error err) {
            mmDevice->Release();
            mmCaptureDeviceCollection->Release();
            throw err;
        }

        adopt(adapter->IInterface::queryInterface<IChild>());
    }

    mmCaptureDeviceCollection->Release();
}

WASAPIInstance::~WASAPIInstance() {
    ParentByVector::disownAll();

    _mmDeviceEnumerator->Release();
    CoUninitialize();
}

/* IInstance */
InstanceBackendFlags WASAPIInstance::backend() const noexcept {
    return InstanceBackendFlags::WASAPI;
}

IAdapter* WASAPIInstance::enumerateAdapters(uint32_t id) const noexcept {
    return IParent::enumerateChildren<IAdapter>(id);
}

/* IInterface */
void* WASAPIInstance::queryInterface(IID const& iid) noexcept {
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
        WASAPIInstance* instance = new WASAPIInstance();
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

namespace wasapi {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

#endif
