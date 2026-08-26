#include <klatr/audio/wasapi/device.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <limits>

#include <klatr/audio/wasapi/buffer.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

WASAPIDevice::WASAPIDevice(IAdapter* adapter, IAudioClient* audioClient, DeviceInfo const& info) : _adapter(adapter), _instance(_adapter->parent<IInstance>()), _audioClient(audioClient), _info(info) {
    _audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&_audioRenderClient));
    _audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&_audioCaptureClient));
}

WASAPIDevice::~WASAPIDevice() {
    stop(DeviceFlowFlags::All);
    _adapter->disown(IInterface::queryInterface<IChild>());

    ParentByVector::disownAll();

    if (_audioRenderClient != nullptr) {
        _audioRenderClient->Release();
    }

    if (_audioCaptureClient != nullptr) {
        _audioCaptureClient->Release();
    }

    _audioClient->Release();
}

/* IOutputDevice */
IOutputBuffer* WASAPIDevice::acquireOutputBuffer(uint32_t frameCount) noexcept {
    if (_audioRenderClient == nullptr) {
        /* TODO: error */
        return nullptr;
    }

    uint64_t formatByteSize = 0;
    switch (_info.format) {
    case FormatFlags::Sint16:
        formatByteSize = 2;
        break;
    case FormatFlags::Sint24:
        formatByteSize = 3;
        break;
    case FormatFlags::Sint32:
    case FormatFlags::Float32:
        formatByteSize = 4;
        break;
    default:
        /* TODO: error */
        return nullptr;
    }

    BYTE* data;
    HRESULT result = _audioRenderClient->GetBuffer(frameCount, &data);
    if (FAILED(result)) {
        /* TODO: error */
        return nullptr;
    }

    WASAPIOutputBuffer* buffer;
    try {
        buffer = new WASAPIOutputBuffer(this, _audioRenderClient, frameCount * formatByteSize, frameCount, reinterpret_cast<void*>(data));
    } catch (std::runtime_error err) {
        /* TODO: error */
        _audioRenderClient->ReleaseBuffer(0, 0);
        return nullptr;
    }

    adopt(buffer);
    return buffer;
}

/* IInputDevice */
IInputBuffer* WASAPIDevice::acquireInputBuffer() noexcept {
    if (_audioCaptureClient == nullptr) {
        /* TODO: error */
        return nullptr;
    }

    uint64_t formatByteSize = 0;
    switch (_info.format) {
    case FormatFlags::Sint16:
        formatByteSize = 2;
        break;
    case FormatFlags::Sint24:
        formatByteSize = 3;
        break;
    case FormatFlags::Sint32:
    case FormatFlags::Float32:
        formatByteSize = 4;
        break;
    default:
        /* TODO: error */
        return nullptr;
    }

    BYTE* data = nullptr;
    UINT32 framesToRead = 0;
    DWORD flags = 0;
    UINT64 timestamp = std::numeric_limits<UINT64>::max();

    HRESULT result = _audioCaptureClient->GetBuffer(&data, &framesToRead, &flags, nullptr, &timestamp);
    if (FAILED(result)) {
        /* TODO: error */
        return nullptr;
    }

    WASAPIInputBuffer* buffer;
    try {
        buffer = new WASAPIInputBuffer(this, _audioCaptureClient, framesToRead * formatByteSize, framesToRead, reinterpret_cast<void*>(data), (result == AUDCLNT_S_BUFFER_EMPTY), (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0, (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) != 0, timestamp);
    } catch (std::runtime_error err) {
        /* TODO: error */
        _audioCaptureClient->ReleaseBuffer(0);
        return nullptr;
    }

    adopt(buffer);
    return buffer;
}

/* IDevice */
void WASAPIDevice::getInfo(DeviceInfo* info) const noexcept {
    std::memcpy(info, &_info, sizeof(DeviceInfo));
}

bool WASAPIDevice::start(DeviceFlowFlags flow) noexcept {
    if ((flow & _info.flow) == DeviceFlowFlags::None) {
        return false;
    }

    HRESULT result = _audioClient->Start();
    return SUCCEEDED(result);
}

bool WASAPIDevice::stop(DeviceFlowFlags flow) noexcept {
    if ((flow & _info.flow) == DeviceFlowFlags::None) {
        return false;
    }

    HRESULT result = _audioClient->Stop();
    return SUCCEEDED(result);
}

uint32_t WASAPIDevice::currentPadding() const noexcept {
    UINT32 padding;
    if (FAILED(_audioClient->GetCurrentPadding(&padding))) {
        return 0;
    }

    return padding;
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

}

}

}

#endif
