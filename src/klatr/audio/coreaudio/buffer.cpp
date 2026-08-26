#include <klatr/audio/coreaudio/buffer.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

CoreAudioOutputBuffer::CoreAudioOutputBuffer(IDevice* device, uint64_t sizeInBytes, uint32_t frameCount, uint8_t* buffer, OutputBufferSignalData& signalData) : _device(device), _adapter(_device->parent<IAdapter>()), _instance(_adapter->parent<IInstance>()), _sizeInBytes(sizeInBytes), _frameCount(frameCount), _buffer(buffer), _signalData(signalData) {
    _device->retain();
}

CoreAudioOutputBuffer::~CoreAudioOutputBuffer() {
    _device->disown(IInterface::queryInterface<IChild>());

    _device->release();
}

/* IOutputBuffer */
void* CoreAudioOutputBuffer::map() noexcept {
    return _buffer;
}

void CoreAudioOutputBuffer::unmap() noexcept {
    /* do nothing machine */
}

void CoreAudioOutputBuffer::silence() noexcept {
    _signalData.silent = true;
}

void CoreAudioOutputBuffer::produce(uint32_t frameCount) noexcept {
    _signalData.producedFrameCount = frameCount;
}

/* IBuffer */
uint64_t CoreAudioOutputBuffer::sizeInBytes() const noexcept {
    return _sizeInBytes;
}

uint32_t CoreAudioOutputBuffer::frameCount()  const noexcept {
    return _frameCount;
}

/* IChild */
IParent* CoreAudioOutputBuffer::parent() const noexcept {
    return _device->queryInterface<IParent>();
}

/* IInterface */
void* CoreAudioOutputBuffer::queryInterface(IID const& iid) noexcept {
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

CoreAudioInputBuffer::CoreAudioInputBuffer(IDevice* device, uint64_t sizeInBytes, uint32_t frameCount, bool empty, bool silent, bool discontinuity, uint64_t timestamp) : _device(device), _adapter(_device->parent<IAdapter>()), _instance(_adapter->parent<IInstance>()), _sizeInBytes(sizeInBytes), _frameCount(frameCount), _empty(empty), _silent(silent), _discontinuity(discontinuity), _timestamp(timestamp) {
    _device->retain();

    _buffer = new uint8_t[sizeInBytes];
}

CoreAudioInputBuffer::~CoreAudioInputBuffer() {
    _device->disown(IInterface::queryInterface<IChild>());

    delete _buffer;

    _device->release();
}

/* IInputBuffer */
void* CoreAudioInputBuffer::map() noexcept {
    return _buffer;
}

void CoreAudioInputBuffer::unmap() noexcept {
    /* do nothing machine */
}

void CoreAudioInputBuffer::consume() noexcept {
    _consumed = true;
}

bool CoreAudioInputBuffer::empty() const noexcept {
    return _empty;
}

bool CoreAudioInputBuffer::silent() const noexcept {
    return _silent;
}

bool CoreAudioInputBuffer::discontinuity() const noexcept {
    return _discontinuity;
}

uint64_t CoreAudioInputBuffer::timestamp() const noexcept {
    return _timestamp;
}

/* IBuffer */
uint64_t CoreAudioInputBuffer::sizeInBytes() const noexcept {
    return _sizeInBytes;
}

uint32_t CoreAudioInputBuffer::frameCount()  const noexcept {
    return _frameCount;
}

/* IChild */
IParent* CoreAudioInputBuffer::parent() const noexcept {
    return _device->queryInterface<IParent>();
}

/* IInterface */
void* CoreAudioInputBuffer::queryInterface(IID const& iid) noexcept {
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
