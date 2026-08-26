#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/buffer.hpp>
#include <klatr/audio/device.hpp>
#include <klatr/audio/adapter.hpp>
#include <klatr/audio/instance.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

class WASAPIOutputBuffer : virtual public IOutputBuffer, virtual public CollectedByHeap {
private:
    IDevice* _device = nullptr;
    IAdapter* _adapter = nullptr;
    IInstance* _instance = nullptr;
    IAudioRenderClient* _audioRenderClient = nullptr;

    uint64_t _sizeInBytes = 0;
    uint32_t _frameCount = 0;
    void* _buffer = nullptr;

    uint32_t _producedFrameCount = 0;
    bool _silent = false;

public:
    WASAPIOutputBuffer(IDevice* device, IAudioRenderClient* audioRenderClient, uint64_t sizeInBytes, uint32_t frameCount, void* buffer);
    ~WASAPIOutputBuffer();

    /* IOutputBuffer */
    void* map() noexcept override;
    void unmap() noexcept override;
    void silence() noexcept override;
    void produce(uint32_t frameCount) noexcept override;

    /* IBuffer */
    uint64_t sizeInBytes() const noexcept override;
    uint32_t frameCount()  const noexcept override;

    /* IChild */
    IParent* parent() const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;
};

class WASAPIInputBuffer : virtual public IInputBuffer, virtual public CollectedByHeap {
private:
    IDevice* _device = nullptr;
    IAdapter* _adapter = nullptr;
    IInstance* _instance = nullptr;
    IAudioCaptureClient* _audioCaptureClient = nullptr;

    uint64_t _sizeInBytes = 0;
    uint32_t _frameCount = 0;
    void* _buffer = nullptr;
    bool _empty = false;
    bool _silent = true;
    bool _discontinuity = false;
    uint64_t _timestamp = 0;

    bool _consumed = false;

public:
    WASAPIInputBuffer(IDevice* device, IAudioCaptureClient* audioCaptureClient, uint64_t sizeInBytes, uint32_t frameCount, void* buffer, bool empty, bool silent, bool discontinuity, uint64_t timestamp);
    ~WASAPIInputBuffer();

    /* IInputBuffer */
    void* map() noexcept override;
    void unmap() noexcept override;
    void consume() noexcept override;

    bool empty() const noexcept override;
    bool silent() const noexcept override;
    bool discontinuity() const noexcept override;
    uint64_t timestamp() const noexcept override;

    /* IBuffer */
    uint64_t sizeInBytes() const noexcept override;
    uint32_t frameCount()  const noexcept override;

    /* IChild */
    IParent* parent() const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;
};

}

}

}

#endif