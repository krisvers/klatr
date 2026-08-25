#include <klatr/audio/wasapi/adapter.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <stdexcept>
#include <string>
#include <cassert>

#include <klatr/audio/wasapi/enums.hpp>
#include <klatr/audio/wasapi/device.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

WASAPIAdapter::WASAPIAdapter(IInstance* instance, IMMDevice* mmDevice, DeviceFlowFlags flow) : _instance(instance), _mmDevice(mmDevice) {
    _instance->retain();

    LPWSTR adapterID;
    assert(SUCCEEDED(mmDevice->GetId(&adapterID)));

    char uuid[37];
    WideCharToMultiByte(CP_UTF8, 0, adapterID, -1, &uuid[0], sizeof(uuid), nullptr, nullptr);
    CoTaskMemFree(adapterID);

    try {
        _info.uuid = kom::UUID(uuid);
    } catch (std::runtime_error err) {
        throw std::runtime_error("Failed to parse IMMDevice endpoint ID");
    }

    IPropertyStore* propertyStore;
    assert(SUCCEEDED(mmDevice->OpenPropertyStore(STGM_READ, &propertyStore)));

    PROPVARIANT channelConfigProp = {};
    assert(SUCCEEDED(propertyStore->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &channelConfigProp)));

    KSAUDIO_CHANNEL_CONFIG channelConfig = {};
    channelConfig.ActiveSpeakerPositions = channelConfigProp.uintVal;

    _info.channels = castEnum(channelConfig);
    assert(SUCCEEDED(PropVariantClear(&channelConfigProp)));

    PROPVARIANT formFactorProp = {};
    assert(SUCCEEDED(propertyStore->GetValue(PKEY_AudioEndpoint_FormFactor, &formFactorProp)));

    _info.transport = castEnum(static_cast<EndpointFormFactor>(formFactorProp.uintVal));
    assert(SUCCEEDED(PropVariantClear(&formFactorProp)));

    _info.flow = flow;

    /* TODO: format detection */
    _info.formats = FormatFlags::Sint16 | FormatFlags::Sint24 | FormatFlags::Sint32 | FormatFlags::Float32;

    PROPVARIANT friendlyNameProp = {};
    assert(SUCCEEDED(propertyStore->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyNameProp)));

    WideCharToMultiByte(CP_UTF8, 0, friendlyNameProp.pwszVal, -1, &_info.name[0], sizeof(_info.name), nullptr, nullptr);
    assert(SUCCEEDED(PropVariantClear(&friendlyNameProp)));

    propertyStore->Release();
}

WASAPIAdapter::~WASAPIAdapter() {
    ParentByVector::disownAll();

    _instance->release();
}

/* IAdapter */
void WASAPIAdapter::getInfo(AdapterInfo* info) const noexcept {
    std::memcpy(info, &_info, sizeof(AdapterInfo));
}

IDevice* WASAPIAdapter::createDevice(DeviceInfo const* info) noexcept {
    return nullptr;
}

/* IParent */
IParent* WASAPIAdapter::parent() const noexcept {
    return _instance->queryInterface<IParent>();
}

/* IInterface */
void* WASAPIAdapter::queryInterface(IID const& iid) noexcept {
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
    } else if (iid == IAdapter::iid()) {
        return static_cast<IAdapter*>(this);
    }

    return nullptr;
}

}

}

}

#endif
