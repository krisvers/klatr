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
    LPWSTR adapterID;
    assert(SUCCEEDED(mmDevice->GetId(&adapterID)));

    char uuid[56];
    WideCharToMultiByte(CP_UTF8, 0, adapterID, -1, &uuid[0], sizeof(uuid), nullptr, nullptr);
    CoTaskMemFree(adapterID);

    uuid[54] = '\0';
    try {
        _info.uuid = kom::UUID(&uuid[18]);
    } catch (std::runtime_error err) {
        throw std::runtime_error("Failed to parse IMMDevice endpoint ID");
    }

    IPropertyStore* propertyStore;
    assert(SUCCEEDED(mmDevice->OpenPropertyStore(STGM_READ, &propertyStore)));

    PROPVARIANT channelConfigProp = {};
    assert(SUCCEEDED(propertyStore->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &channelConfigProp)));

    KSAUDIO_CHANNEL_CONFIG channelConfig = {};
    channelConfig.ActiveSpeakerPositions = channelConfigProp.uintVal;

    if (channelConfigProp.vt == VT_EMPTY) {
        /* default to stereo if property store succeeds with empty value (for some reason not documented?) */
        _info.channels = DeviceChannelFlags::Stereo;
    } else {
        _info.channels = castEnum(channelConfig);
    }

    assert(SUCCEEDED(PropVariantClear(&channelConfigProp)));

    PROPVARIANT formFactorProp = {};
    assert(SUCCEEDED(propertyStore->GetValue(PKEY_AudioEndpoint_FormFactor, &formFactorProp)));

    _info.transport = castEnum(static_cast<EndpointFormFactor>(formFactorProp.uintVal));
    assert(SUCCEEDED(PropVariantClear(&formFactorProp)));

    _info.flow = flow;

    IAudioClient* dummyClient;
    HRESULT result = mmDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&dummyClient));
    assert(SUCCEEDED(result));

    uint32_t typicalSampleRates[12] = {
        8000,
        11025,
        16000,
        22050,
        44100,
        48000,
        88200,
        96000,
        176400,
        192000,
        352800,
        384000,
    };

    FormatFlags supportedFormatsAtTypicalSampleRates[12] = {};

    FormatFlags totalFormats = FormatFlags::None;
    uint32_t lowestIndex = 11;
    uint32_t highestIndex = 0;
    for (uint32_t i = 0; i < 12; i += 1) {
        WAVEFORMATEXTENSIBLE wfx = {};
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.Format.nChannels = ((_info.channels == DeviceChannelFlags::Mono) ? 1 : 2);
        wfx.Format.nSamplesPerSec = typicalSampleRates[i];
        wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

        /* Sint16 */
        wfx.Format.wBitsPerSample = 16;
        wfx.Format.nBlockAlign = wfx.Format.nChannels * (wfx.Format.wBitsPerSample >> 3);
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        WAVEFORMATEXTENSIBLE* closestMatch;
        HRESULT result = dummyClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &wfx.Format, reinterpret_cast<WAVEFORMATEX**>(&closestMatch));
        if (SUCCEEDED(result)) {
            if (result == S_OK) {
                supportedFormatsAtTypicalSampleRates[i] |= FormatFlags::Sint16;
            }

            CoTaskMemFree(closestMatch);
        }

        /* Sint24 */
        wfx.Format.wBitsPerSample = 24;
        wfx.Format.nBlockAlign = wfx.Format.nChannels * (wfx.Format.wBitsPerSample >> 3);
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        result = dummyClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &wfx.Format, reinterpret_cast<WAVEFORMATEX**>(&closestMatch));
        if (SUCCEEDED(result)) {
            if (result == S_OK) {
                supportedFormatsAtTypicalSampleRates[i] |= FormatFlags::Sint24;
            }

            CoTaskMemFree(closestMatch);
        }

        /* Sint32 */
        wfx.Format.wBitsPerSample = 32;
        wfx.Format.nBlockAlign = wfx.Format.nChannels * (wfx.Format.wBitsPerSample >> 3);
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        result = dummyClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &wfx.Format, reinterpret_cast<WAVEFORMATEX**>(&closestMatch));
        if (SUCCEEDED(result)) {
            if (result == S_OK) {
                supportedFormatsAtTypicalSampleRates[i] |= FormatFlags::Sint32;
            }

            CoTaskMemFree(closestMatch);
        }

        /* Float32 */
        wfx.Format.wBitsPerSample = 32;
        wfx.Format.nBlockAlign = wfx.Format.nChannels * (wfx.Format.wBitsPerSample >> 3);
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

        result = dummyClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &wfx.Format, reinterpret_cast<WAVEFORMATEX**>(&closestMatch));
        if (SUCCEEDED(result)) {
            if (result == S_OK) {
                supportedFormatsAtTypicalSampleRates[i] |= FormatFlags::Float32;
            }

            CoTaskMemFree(closestMatch);
        }

        if (supportedFormatsAtTypicalSampleRates[i] != FormatFlags::None) {
            if (i > highestIndex) {
                highestIndex = i;
            }

            if (i < lowestIndex) {
                lowestIndex = i;
            }
        }

        totalFormats |= supportedFormatsAtTypicalSampleRates[i];
    }

    dummyClient->Release();

    _info.formats = totalFormats;

    _info.lowestTypicalSampleRate = typicalSampleRates[lowestIndex];
    _info.highestTypicalSampleRate = typicalSampleRates[highestIndex];

    PROPVARIANT friendlyNameProp = {};
    assert(SUCCEEDED(propertyStore->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyNameProp)));

    WideCharToMultiByte(CP_UTF8, 0, friendlyNameProp.pwszVal, -1, &_info.name[0], sizeof(_info.name), nullptr, nullptr);
    assert(SUCCEEDED(PropVariantClear(&friendlyNameProp)));

    propertyStore->Release();
}

WASAPIAdapter::~WASAPIAdapter() {
    _instance->disown(IInterface::queryInterface<IChild>());

    ParentByVector::disownAll();
}

/* IAdapter */
void WASAPIAdapter::getInfo(AdapterInfo* info) const noexcept {
    std::memcpy(info, &_info, sizeof(AdapterInfo));
}

IDevice* WASAPIAdapter::createDevice(DeviceInfo const* info) noexcept {
    WAVEFORMATEXTENSIBLE wfx = {};
    wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.nChannels = ((_info.channels == DeviceChannelFlags::Mono) ? 1 : 2);
    wfx.Format.nSamplesPerSec = info->sampleRate;
    wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

    FormatFlags format = (info->format & _info.formats);

    if ((format & FormatFlags::Float32) != FormatFlags::None) {
        wfx.Format.wBitsPerSample = 32;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    } else if ((format & FormatFlags::Sint32) != FormatFlags::None) {
        wfx.Format.wBitsPerSample = 32;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    } else if ((format & FormatFlags::Sint24) != FormatFlags::None) {
        wfx.Format.wBitsPerSample = 24;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    } else if ((format & FormatFlags::Sint16) != FormatFlags::None) {
        wfx.Format.wBitsPerSample = 16;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    } else {
        /* no supported format found */
        return nullptr;
    }

    wfx.Format.nBlockAlign = wfx.Format.nChannels * (wfx.Format.wBitsPerSample >> 3);
    wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
    wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;

    IAudioClient* client;
    HRESULT result = _mmDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&client));
    if (FAILED(result)) {
        return nullptr;
    }

    result = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, static_cast<REFERENCE_TIME>(static_cast<uint64_t>(info->sampleCount) * static_cast<uint64_t>(10000000) / info->sampleRate), 0, &wfx.Format, nullptr);
    if (FAILED(result)) {
        client->Release();
        return nullptr;
    }

    WASAPIDevice* device;
    try {
        device = new WASAPIDevice(this, client, *info);
    } catch (std::runtime_error err) {
        client->Release();
        return nullptr;
    }

    adopt(device->IInterface::queryInterface<IChild>());
    return device->IInterface::queryInterface<IDevice>();
}

/* IParent */
IParent* WASAPIAdapter::parent() const noexcept {
    return _instance->queryInterface<IParent>();
}

/* IInterface */
void* WASAPIAdapter::queryInterface(IID const& iid) noexcept {
    if (iid == IBase::iid()) {
        return static_cast<IBase*>(this);
    } else if (iid == IParent::iid()) {
        return static_cast<IParent*>(this);
    } else if (iid == IChild::iid()) {
        return static_cast<IChild*>(this);
    } else if (iid == IAudioElement::iid()) {
        return static_cast<IAudioElement*>(this);
    } else if (iid == IAdapter::iid()) {
        return static_cast<IAdapter*>(this);
    } else if (iid == IOutputAdapter::iid()) {
        if ((_info.flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) {
            return static_cast<IOutputAdapter*>(this);
        }

        return nullptr;
    } else if (iid == IInputAdapter::iid()) {
        if ((_info.flow & DeviceFlowFlags::Input) != DeviceFlowFlags::None) {
            return static_cast<IInputAdapter*>(this);
        }

        return nullptr;
    }

    return nullptr;
}

}

}

}

#endif
