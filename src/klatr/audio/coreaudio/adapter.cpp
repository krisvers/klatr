#include <klatr/audio/coreaudio/adapter.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <functional>

#include <klatr/audio/coreaudio/enums.hpp>
#include <klatr/audio/coreaudio/device.hpp>

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

CoreAudioAdapter::CoreAudioAdapter(IInstance* instance, AudioDeviceID deviceID) : _instance(instance), _deviceID(deviceID) {
    AudioObjectPropertyAddress deviceInputStreamConfigurationProp = {};
    deviceInputStreamConfigurationProp.mSelector = kAudioDevicePropertyStreamConfiguration;
    deviceInputStreamConfigurationProp.mScope = kAudioObjectPropertyScopeInput;
    deviceInputStreamConfigurationProp.mElement = kAudioObjectPropertyElementMain;

    UInt32 deviceInputStreamConfigurationSize = 0;
    if (AudioObjectGetPropertyDataSize(_deviceID, &deviceInputStreamConfigurationProp, 0, nullptr, &deviceInputStreamConfigurationSize) == noErr && deviceInputStreamConfigurationSize > 0) {
        _info.flow |= DeviceFlowFlags::Input;
    }

    AudioObjectPropertyAddress deviceOutputStreamConfigurationProp = {};
    deviceOutputStreamConfigurationProp.mSelector = kAudioDevicePropertyStreamConfiguration;
    deviceOutputStreamConfigurationProp.mScope = kAudioObjectPropertyScopeOutput;
    deviceOutputStreamConfigurationProp.mElement = kAudioObjectPropertyElementMain;

    UInt32 deviceOutputStreamConfigurationSize = 0;
    if (AudioObjectGetPropertyDataSize(_deviceID, &deviceOutputStreamConfigurationProp, 0, nullptr, &deviceOutputStreamConfigurationSize) == noErr && deviceOutputStreamConfigurationSize > 0) {
        _info.flow |= DeviceFlowFlags::Output;
    }

    UInt32 flowSpecificScope = 0;
    if (_info.flow == (DeviceFlowFlags::Output | DeviceFlowFlags::Input)) {
        flowSpecificScope = kAudioObjectPropertyScopeGlobal;
    } else if (_info.flow == DeviceFlowFlags::Output) {
        flowSpecificScope = kAudioObjectPropertyScopeOutput;
    } else if (_info.flow == DeviceFlowFlags::Input) {
        flowSpecificScope = kAudioObjectPropertyScopeInput;
    } else {
        /* should not be possible? */
        throw std::runtime_error("Internal flow flags don't make sense");
    }

    AudioObjectPropertyAddress defaultInputDeviceProp = {};
    defaultInputDeviceProp.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    defaultInputDeviceProp.mScope = kAudioObjectPropertyScopeGlobal;
    defaultInputDeviceProp.mElement = kAudioObjectPropertyElementMain;

    AudioDeviceID defaultInputDevice = 0;
    UInt32 defaultInputDevicePropSize = sizeof(AudioDeviceID);
    OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultInputDeviceProp, 0, nullptr, &defaultInputDevicePropSize, &defaultInputDevice);
    assert(status == noErr);

    AudioObjectPropertyAddress defaultOutputDeviceProp = {};
    defaultOutputDeviceProp.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    defaultOutputDeviceProp.mScope = kAudioObjectPropertyScopeGlobal;
    defaultOutputDeviceProp.mElement = kAudioObjectPropertyElementMain;

    AudioDeviceID defaultOutputDevice = 0;
    UInt32 defaultOutputDevicePropSize = sizeof(AudioDeviceID);
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultOutputDeviceProp, 0, nullptr, &defaultOutputDevicePropSize, &defaultOutputDevice);
    assert(status == noErr);

    if (deviceID == defaultInputDevice) {
        _info.defaultForFlow = DeviceFlowFlags::Input;
    }

    if (deviceID == defaultOutputDevice) {
        _info.defaultForFlow = DeviceFlowFlags::Output;
    }

    /* NOTE: known problem with kAudioDevicePropertyDeviceUID not being persistent via USB audio(?) */
    AudioObjectPropertyAddress deviceUIDProp = {};
    deviceUIDProp.mSelector = kAudioDevicePropertyDeviceUID;
    deviceUIDProp.mScope = kAudioObjectPropertyScopeGlobal;
    deviceUIDProp.mElement = kAudioObjectPropertyElementMain;

    CFStringRef deviceUID = nullptr;
    UInt32 deviceUIDSize = sizeof(CFStringRef);
    status = AudioObjectGetPropertyData(_deviceID, &deviceUIDProp, 0, nullptr, &deviceUIDSize, &deviceUID);
    assert(status == noErr);

    char uuid[64];
    assert(CFStringGetCString(deviceUID, uuid, sizeof(uuid), kCFStringEncodingUTF8));

    /* genuine v3/v5-esque nonsense to hash string into UUID (need to implement something better) */
    std::hash<std::string_view> hasher = {};
    uint64_t hashes[2];
    hashes[0] = hasher(std::string_view(&uuid[0], 16));
    hashes[1] = hasher(std::string_view(&uuid[4], std::strlen(uuid) - 4));

    CFRelease(deviceUID);

    uint8_t* uuidBytes = reinterpret_cast<uint8_t*>(&hashes[0]);
    uuidBytes[6] = (uuidBytes[6] & 0x0f) | 0x50; /* lying that this is v5 */
    uuidBytes[8] = (uuidBytes[8] & 0x3f) | 0x80; /* RFC 4122 */

    _info.uuid = kom::UUID(uuidBytes);

    AudioObjectPropertyAddress deviceChannelLayoutProp = {};
    deviceChannelLayoutProp.mSelector = kAudioDevicePropertyPreferredChannelLayout;
    deviceChannelLayoutProp.mScope = flowSpecificScope;
    deviceChannelLayoutProp.mElement = kAudioObjectPropertyElementMain;

    AudioChannelLayout deviceChannelLayout = {};
    UInt32 deviceChannelLayoutSize = sizeof(deviceChannelLayout);
    status = AudioObjectGetPropertyData(_deviceID, &deviceChannelLayoutProp, 0, nullptr, &deviceChannelLayoutSize, &deviceChannelLayout);
    if (status == noErr) {
        _info.channels = castEnum(deviceChannelLayout.mChannelBitmap);
    } else {
        /* TODO: some form of AudioUnit fallback maybe? */
        _info.channels = DeviceChannelFlags::Stereo;
    }

    AudioObjectPropertyAddress deviceTransportTypeProp = {};
    deviceTransportTypeProp.mSelector = kAudioDevicePropertyTransportType;
    deviceTransportTypeProp.mScope = (((_info.flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) ? kAudioObjectPropertyScopeOutput : kAudioObjectPropertyScopeInput);
    deviceTransportTypeProp.mElement = kAudioObjectPropertyElementMain;

    UInt32 deviceTransportTypeUint;
    UInt32 deviceTransportTypeSize = sizeof(UInt32);
    status = AudioObjectGetPropertyData(_deviceID, &deviceTransportTypeProp, 0, nullptr, &deviceTransportTypeSize, &deviceTransportTypeUint);
    assert(status == noErr);

    /* stupid wrapper type for castEnum<T> type distinction */
    CoreAudioDeviceTransportType deviceTransportType = {};
    deviceTransportType.type = deviceTransportTypeUint;

    _info.transport = castEnum(deviceTransportType);

    AudioObjectPropertyAddress streamIDsProp = {};
    streamIDsProp.mSelector = kAudioDevicePropertyStreams;
    streamIDsProp.mScope = kAudioObjectPropertyScopeGlobal;
    streamIDsProp.mElement = kAudioObjectPropertyElementMain;

    UInt32 streamIDBufferSize = 0;
    status = AudioObjectGetPropertyDataSize(_deviceID, &streamIDsProp, 0, nullptr, &streamIDBufferSize);
    assert(status == noErr);

    std::vector<AudioStreamID> streamIDs(streamIDBufferSize / sizeof(AudioStreamID));
    if (!streamIDs.empty()) {
        status = AudioObjectGetPropertyData(_deviceID, &streamIDsProp, 0, nullptr, &streamIDBufferSize, &streamIDs[0]);
        assert(status == noErr);

        AudioObjectPropertyAddress deviceStreamFormatProp = {};
        deviceStreamFormatProp.mSelector = kAudioDevicePropertyStreamFormat;
        deviceStreamFormatProp.mScope = (((_info.flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) ? kAudioObjectPropertyScopeOutput : kAudioObjectPropertyScopeInput); //flowSpecificScope;
        deviceStreamFormatProp.mElement = kAudioObjectPropertyElementMain;

        AudioStreamBasicDescription deviceStreamFormat;
        UInt32 deviceStreamFormatSize = sizeof(AudioStreamBasicDescription);
        status = AudioObjectGetPropertyData(streamIDs[0], &deviceStreamFormatProp, 0, nullptr, &deviceStreamFormatSize, &deviceStreamFormat);
        assert(status == noErr);

        if (deviceStreamFormat.mBitsPerChannel == 32) {
            if ((deviceStreamFormat.mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0) {
                _info.formats |= FormatFlags::Sint32;
            } else if ((deviceStreamFormat.mFormatFlags & kAudioFormatFlagIsFloat) != 0) {
                _info.formats |= FormatFlags::Float32;
            }
        } else if (deviceStreamFormat.mBitsPerChannel == 24 && (deviceStreamFormat.mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0) {
            _info.formats |= FormatFlags::Sint24;
        } else if (deviceStreamFormat.mBitsPerChannel == 16 && (deviceStreamFormat.mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0) {
            _info.formats |= FormatFlags::Sint16;
        }

        _info.lowestTypicalSampleRate = deviceStreamFormat.mSampleRate;
        _info.highestTypicalSampleRate = deviceStreamFormat.mSampleRate;
    } else {
        /* TODO: fallback idfk atp */
    }

    AudioObjectPropertyAddress deviceNameProp = {};
    deviceNameProp.mSelector = kAudioObjectPropertyName;
    deviceNameProp.mScope = kAudioObjectPropertyScopeGlobal;
    deviceNameProp.mElement = kAudioObjectPropertyElementMain;

    CFStringRef deviceName = nullptr;
    UInt32 deviceNameSize = sizeof(CFStringRef);
    status = AudioObjectGetPropertyData(_deviceID, &deviceNameProp, 0, nullptr, &deviceNameSize, &deviceName);
    assert(status == noErr);

    assert(CFStringGetCString(deviceName, _info.name, sizeof(_info.name), kCFStringEncodingUTF8));
    CFRelease(deviceName);
}

CoreAudioAdapter::~CoreAudioAdapter() {
    _instance->disown(IInterface::queryInterface<IChild>());

    ParentByVector::disownAll();
}

/* IAdapter */
void CoreAudioAdapter::getInfo(AdapterInfo* info) const noexcept {
    std::memcpy(info, &_info, sizeof(AdapterInfo));
}

IDevice* CoreAudioAdapter::createDevice(DeviceInfo const* info) noexcept {
    CoreAudioDevice* device;
    try {
        device = new CoreAudioDevice(this, _deviceID, *info);
    } catch (std::runtime_error err) {
        return nullptr;
    }

    adopt(device->IInterface::queryInterface<IChild>());
    return device->IInterface::queryInterface<IDevice>();
}

/* IParent */
IParent* CoreAudioAdapter::parent() const noexcept {
    return _instance->queryInterface<IParent>();
}

/* IInterface */
void* CoreAudioAdapter::queryInterface(IID const& iid) noexcept {
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
