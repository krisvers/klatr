#include <klatr/audio/wasapi/buffer.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

WASAPIOutputBuffer::WASAPIOutputBuffer(IDevice* device, IAudioRenderClient* audioRenderClient, uint64_t sizeInBytes, uint32_t frameCount, void* buffer) : _device(device), _adapter(_device->parent<IAdapter>()), _instance(_adapter->parent<IInstance>()), _audioRenderClient(audioRenderClient), _sizeInBytes(sizeInBytes), _frameCount(frameCount), _buffer(buffer) {
    _device->retain();
}

WASAPIOutputBuffer::~WASAPIOutputBuffer() {
    _device->disown(IInterface::queryInterface<IChild>());

    _audioRenderClient->ReleaseBuffer(_producedFrameCount, (_silent ? AUDCLNT_BUFFERFLAGS_SILENT : 0));

    _device->release();
}

/* IOutputBuffer */
void* WASAPIOutputBuffer::map() noexcept {
    return _buffer;
}

void WASAPIOutputBuffer::unmap() noexcept {
    /* do nothing machine */
}

void WASAPIOutputBuffer::silence() noexcept {
    _silent = true;
}

void WASAPIOutputBuffer::produce(uint32_t frameCount) noexcept {
    _producedFrameCount = frameCount;
}

/* IBuffer */
uint64_t WASAPIOutputBuffer::sizeInBytes() const noexcept {
    return _sizeInBytes;
}

uint32_t WASAPIOutputBuffer::frameCount()  const noexcept {
    return _frameCount;
}

/* IChild */
IParent* WASAPIOutputBuffer::parent() const noexcept {
    return _device->queryInterface<IParent>();
}

/* IInterface */
void* WASAPIOutputBuffer::queryInterface(IID const& iid) noexcept {
    if (iid == IBase::iid()) {
        return static_cast<IBase*>(this);
    } else if (iid == ICollected::iid()) {
        return static_cast<ICollected*>(this);
    } else if (iid == IChild::iid()) {
        return static_cast<IChild*>(this);
    } else if (iid == IAudioElement::iid()) {
        return static_cast<IAudioElement*>(this);
    } else if (iid == IBuffer::iid()) {
        return static_cast<IBuffer*>(this);
    } else if (iid == IOutputBuffer::iid()) {
        return static_cast<IOutputBuffer*>(this);
    }

    return nullptr;
}

WASAPIInputBuffer::WASAPIInputBuffer(IDevice* device, IAudioCaptureClient* audioCaptureClient, uint64_t sizeInBytes, uint32_t frameCount, void* buffer, bool empty, bool silent, bool discontinuity, uint64_t timestamp) : _device(device), _adapter(_device->parent<IAdapter>()), _instance(_adapter->parent<IInstance>()), _audioCaptureClient(audioCaptureClient), _sizeInBytes(sizeInBytes), _frameCount(frameCount), _buffer(buffer), _empty(empty), _silent(silent), _discontinuity(discontinuity), _timestamp(timestamp) {
    _device->retain();
}

WASAPIInputBuffer::~WASAPIInputBuffer() {
    _device->disown(IInterface::queryInterface<IChild>());

    _audioCaptureClient->ReleaseBuffer(_consumed ? _frameCount : 0);

    _device->release();
}

/* IInputBuffer */
void* WASAPIInputBuffer::map() noexcept {
    return _buffer;
}

void WASAPIInputBuffer::unmap() noexcept {
    /* do nothing machine */
}

void WASAPIInputBuffer::consume() noexcept {
    _consumed = true;
}

bool WASAPIInputBuffer::empty() const noexcept {
    return _empty;
}

bool WASAPIInputBuffer::silent() const noexcept {
    return _silent;
}

bool WASAPIInputBuffer::discontinuity() const noexcept {
    return _discontinuity;
}

uint64_t WASAPIInputBuffer::timestamp() const noexcept {
    return _timestamp;
}

/* IBuffer */
uint64_t WASAPIInputBuffer::sizeInBytes() const noexcept {
    return _sizeInBytes;
}

uint32_t WASAPIInputBuffer::frameCount()  const noexcept {
    return _frameCount;
}

/* IChild */
IParent* WASAPIInputBuffer::parent() const noexcept {
    return _device->queryInterface<IParent>();
}

/* IInterface */
void* WASAPIInputBuffer::queryInterface(IID const& iid) noexcept {
    if (iid == IBase::iid()) {
        return static_cast<IBase*>(this);
    } else if (iid == ICollected::iid()) {
        return static_cast<ICollected*>(this);
    } else if (iid == IChild::iid()) {
        return static_cast<IChild*>(this);
    } else if (iid == IAudioElement::iid()) {
        return static_cast<IAudioElement*>(this);
    } else if (iid == IBuffer::iid()) {
        return static_cast<IBuffer*>(this);
    } else if (iid == IInputBuffer::iid()) {
        return static_cast<IInputBuffer*>(this);
    }

    return nullptr;
}

}

}

}

#endif