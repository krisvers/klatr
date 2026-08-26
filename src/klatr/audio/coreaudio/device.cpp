#include <klatr/audio/coreaudio/device.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

CoreAudioDevice::CoreAudioDevice(IAdapter* adapter, AudioDeviceID deviceID, DeviceInfo const& info) : _adapter(adapter), _instance(_adapter->parent<IInstance>()), _deviceID(deviceID), _info(info) {
    if ((_info.flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) {
        AudioComponentDescription outputComponentDescription = {};
        outputComponentDescription.componentType = kAudioUnitType_Output;
        outputComponentDescription.componentSubType = kAudioUnitSubType_HALOutput;
        outputComponentDescription.componentManufacturer = kAudioUnitManufacturer_Apple;

        AudioComponent outputComponent = AudioComponentFindNext(nullptr, &outputComponentDescription);
        if (outputComponent == nullptr) {
            throw std::runtime_error("AudioComponentFindNext for kAudioUnitSubType_HALOutput failed");
        }

        OSStatus status = AudioComponentInstanceNew(outputComponent, &_outputAudioUnit);
        assert(status == noErr);

        status = AudioUnitSetProperty(_outputAudioUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Output, 0, &_deviceID, sizeof(AudioDeviceID));
        assert(status == noErr);

        AURenderCallbackStruct renderCallback = {};
        renderCallback.inputProc = renderAudioCallback;
        renderCallback.inputProcRefCon = this;

        status = AudioUnitSetProperty(_outputAudioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &renderCallback, sizeof(renderCallback));
        assert(status == noErr);

        status = AudioUnitInitialize(_outputAudioUnit);
        assert(status == noErr);

        _outputBufferRaw = new uint8_t[_info.sampleCount * formatByteSize()];
    }
}

CoreAudioDevice::~CoreAudioDevice() {
    stop(DeviceFlowFlags::All);
    _adapter->disown(IInterface::queryInterface<IChild>());

    ParentByVector::disownAll();

    if (_outputAudioUnit != nullptr) {
        AudioUnitUninitialize(_outputAudioUnit);
        AudioComponentInstanceDispose(_outputAudioUnit);
    }

    delete[] _outputBufferRaw;
}

/* IOutputDevice */
IOutputBuffer* CoreAudioDevice::acquireOutputBuffer(uint32_t frameCount) noexcept {
    if (_outputAudioUnit == nullptr) {
        /* TODO: error */
        return nullptr;
    }

    CoreAudioOutputBuffer* buffer;
    try {
        _previousOutputSignalData = _currentOutputSignalData;
        _currentOutputSignalData = {};
        buffer = new CoreAudioOutputBuffer(this, frameCount * formatByteSize(), frameCount, _outputBufferRaw, _currentOutputSignalData);
    } catch (std::runtime_error err) {
        return nullptr;
    }

    adopt(buffer->IInterface::queryInterface<IChild>());
    return buffer->IInterface::queryInterface<IOutputBuffer>();
}

/* IInputDevice */
IInputBuffer* CoreAudioDevice::acquireInputBuffer() noexcept {
    //if (_audioCaptureClient == nullptr) {
    //    /* TODO: error */
    //    return nullptr;
    //}

    /* TODO:
    CoreAudioInputBuffer* buffer;
    try {
        buffer = new CoreAudioInputBuffer(this, frameCount * formatByteSize, frameCount);
    } catch (std::runtime_error err) {
        return nullptr;
    }

    adopt(buffer->IInterface::queryInterface<IChild>());
    return buffer->IInterface::queryInterface<IInputBuffer>();
    */
    return nullptr;
}

/* IDevice */
void CoreAudioDevice::getInfo(DeviceInfo* info) const noexcept {
    std::memcpy(info, &_info, sizeof(DeviceInfo));
}

bool CoreAudioDevice::start(DeviceFlowFlags flow) noexcept {
    bool success = true;
    if (_outputAudioUnit != nullptr && (flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) {
        success = success && (AudioOutputUnitStart(_outputAudioUnit) != noErr);
    }

    /* TODO: input
    if (_outputAudioUnit != nullptr && (flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) {
        success = success && (AudioOutputUnitStart(_outputAudioUnit) != noErr);
    }
    */

    return success;
}

bool CoreAudioDevice::stop(DeviceFlowFlags flow) noexcept {
    bool success = true;
    if (_outputAudioUnit != nullptr && (flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) {
        success = success && (AudioOutputUnitStop(_outputAudioUnit) != noErr);
    }

    /* TODO: input
    if (_outputAudioUnit != nullptr && (flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) {
        success = success && (AudioOutputUnitStop(_outputAudioUnit) != noErr);
    }
    */

    return success;
}

uint32_t CoreAudioDevice::currentPadding() const noexcept {
    /* TODO: ? */
    return 0;
}

/* IParent */
IParent* CoreAudioDevice::parent() const noexcept {
    return _adapter->queryInterface<IParent>();
}

/* IInterface */
void* CoreAudioDevice::queryInterface(IID const& iid) noexcept {
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
    } else if (iid == IOutputDevice::iid()) {
        if ((_info.flow & DeviceFlowFlags::Output) != DeviceFlowFlags::None) {
            return static_cast<IOutputDevice*>(this);
        }

        return nullptr;
    } else if (iid == IInputDevice::iid()) {
        if ((_info.flow & DeviceFlowFlags::Input) != DeviceFlowFlags::None) {
            return static_cast<IInputDevice*>(this);
        }

        return nullptr;
    }

    return nullptr;
}

/* internal */
OSStatus CoreAudioDevice::renderAudioCallback(void* refCon, AudioUnitRenderActionFlags* actionFlags, AudioTimeStamp const* timestamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* data) {
    CoreAudioDevice* device = reinterpret_cast<CoreAudioDevice*>(refCon);
    std::memcpy(data->mBuffers[0].mData, device->_outputBufferRaw, std::min(frameCount, device->_previousOutputSignalData.producedFrameCount) * device->channelCount() * device->formatByteSize());
    return noErr;
}

}

}

}

#endif
