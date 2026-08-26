#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <bitset>

#include <klatr/audio/device.hpp>
#include <klatr/audio/adapter.hpp>
#include <klatr/audio/instance.hpp>

#include <klatr/audio/coreaudio/buffer.hpp>

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

/* NOTE: while this implements IOutputDevice and IInputDevice, only
    instances that support each flow will advertise their interfaces
*/

class CoreAudioDevice : virtual public IDevice, virtual public IOutputDevice, virtual public IInputDevice, virtual public CollectedByHeap, virtual public ParentByVector {
private:
    IAdapter* _adapter = nullptr;
    IInstance* _instance = nullptr;
    AudioDeviceID _deviceID = 0;
    DeviceInfo _info = {};

    AudioUnit _outputAudioUnit = nullptr;

    uint8_t* _outputBufferRaw = nullptr;
    OutputBufferSignalData _previousOutputSignalData = {};
    OutputBufferSignalData _currentOutputSignalData = {};
    CoreAudioOutputBuffer* _currentOutputBuffer = nullptr;

    static OSStatus renderAudioCallback(void* refCon, AudioUnitRenderActionFlags* actionFlags, AudioTimeStamp const* timestamp, UInt32 busNumber, UInt32 frameCount, AudioBufferList* data);

    inline uint32_t channelCount() {
        AdapterInfo adapterInfo = {};
        _adapter->getInfo(&adapterInfo);

        std::bitset<32> bitset(static_cast<std::underlying_type_t<DeviceChannelFlags>>(adapterInfo.channels));
        return bitset.count();
    }

    inline uint64_t formatByteSize() {
        switch (_info.format) {
            case FormatFlags::Sint16:
                return 2;
            case FormatFlags::Sint24:
                return 3;
            case FormatFlags::Sint32:
            case FormatFlags::Float32:
                return 4;
            default:
                break;
        }

        return 0;
    }

public:
    CoreAudioDevice(IAdapter* adapter, AudioDeviceID deviceID, DeviceInfo const& info);
    ~CoreAudioDevice();

    /* IOutputDevice */
    IOutputBuffer* acquireOutputBuffer(uint32_t frameCount) noexcept override;

    /* IInputDevice */
    IInputBuffer* acquireInputBuffer() noexcept override;

    /* IDevice */
    void getInfo(DeviceInfo* info) const noexcept override;

    bool start(DeviceFlowFlags flow) noexcept override;
    bool stop(DeviceFlowFlags flow) noexcept override;

    uint32_t currentPadding() const noexcept override;

    /* IChild */
    IParent* parent() const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;
};

}

}

}

#endif
