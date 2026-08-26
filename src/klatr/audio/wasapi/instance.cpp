#include <klatr/audio/wasapi/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <stdexcept>

#include <klatr/audio/wasapi/adapter.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

WASAPIInstance::WASAPIInstance() {
    HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(result)) {
        throw std::runtime_error("CoInitializeEx failed");
    }

    result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&_mmDeviceEnumerator));
    if (FAILED(result)) {
        throw std::runtime_error("CoCreateInstance for IMMDeviceEnumerator failed");
    }

    IMMDeviceCollection* mmRenderDeviceCollection;
    result = _mmDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &mmRenderDeviceCollection);
    if (FAILED(result)) {
        throw std::runtime_error("IMMDeviceEnumerator::EnumAudioEndpoints failed");
    }

    UINT renderDeviceCount;
    result = mmRenderDeviceCollection->GetCount(&renderDeviceCount);
    if (FAILED(result)) {
        mmRenderDeviceCollection->Release();
        throw std::runtime_error("IMMDeviceCollection::GetCount failed");
    }

    for (UINT i = 0; i < renderDeviceCount; i += 1) {
        IMMDevice* mmDevice;
        result = mmRenderDeviceCollection->Item(i, &mmDevice);
        if (FAILED(result)) {
            mmRenderDeviceCollection->Release();
            throw std::runtime_error("IMMDeviceCollection::Item failed");
        }

        WASAPIAdapter* adapter;
        try {
            adapter = new WASAPIAdapter(this, mmDevice, DeviceFlowFlags::Output);
        } catch (std::runtime_error err) {
            for (WASAPIAdapter* adapter : _adapters) {
                delete adapter;
            }

            mmDevice->Release();
            mmRenderDeviceCollection->Release();
            throw err;
        }

        _adapters.push_back(adapter);
        adopt(adapter->IInterface::queryInterface<IChild>());
    }

    mmRenderDeviceCollection->Release();

    IMMDeviceCollection* mmCaptureDeviceCollection;
    result = _mmDeviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &mmCaptureDeviceCollection);
    if (FAILED(result)) {
        for (WASAPIAdapter* adapter : _adapters) {
            delete adapter;
        }

        throw std::runtime_error("IMMDeviceEnumerator::EnumAudioEndpoints failed");
    }

    UINT captureDeviceCount;
    result = mmCaptureDeviceCollection->GetCount(&captureDeviceCount);
    if (FAILED(result)) {
        for (WASAPIAdapter* adapter : _adapters) {
            delete adapter;
        }

        mmCaptureDeviceCollection->Release();
        throw std::runtime_error("IMMDeviceCollection::GetCount failed");
    }

    for (UINT i = 0; i < captureDeviceCount; i += 1) {
        IMMDevice* mmDevice;
        result = mmCaptureDeviceCollection->Item(i, &mmDevice);
        if (FAILED(result)) {
            for (WASAPIAdapter* adapter : _adapters) {
                delete adapter;
            }

            mmCaptureDeviceCollection->Release();
            throw std::runtime_error("IMMDeviceCollection::Item failed");
        }

        WASAPIAdapter* adapter;
        try {
            adapter = new WASAPIAdapter(this, mmDevice, DeviceFlowFlags::Input);
        } catch (std::runtime_error err) {
            for (WASAPIAdapter* adapter : _adapters) {
                delete adapter;
            }

            mmDevice->Release();
            mmCaptureDeviceCollection->Release();
            throw err;
        }

        _adapters.push_back(adapter);
        adopt(adapter->IInterface::queryInterface<IChild>());
    }

    mmCaptureDeviceCollection->Release();
}

WASAPIInstance::~WASAPIInstance() {
    ParentByVector::disownAll();

    for (WASAPIAdapter* adapter : _adapters) {
        delete adapter;
    }

    _mmDeviceEnumerator->Release();
    CoUninitialize();
}

/* IInstance */
InstanceBackendFlags WASAPIInstance::backend() const noexcept {
    return InstanceBackendFlags::WASAPI;
}

IAdapter* WASAPIInstance::enumerateAdapters(uint32_t id, IID const& filter) const noexcept {
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

namespace wasapi {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

#endif
