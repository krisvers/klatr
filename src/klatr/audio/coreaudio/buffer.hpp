#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/buffer.hpp>
#include <klatr/audio/device.hpp>
#include <klatr/audio/adapter.hpp>
#include <klatr/audio/instance.hpp>

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

struct OutputBufferSignalData {
    uint32_t producedFrameCount;
    bool silent;
};

class CoreAudioOutputBuffer : virtual public IOutputBuffer, virtual public CollectedByHeap {
private:
    IDevice* _device = nullptr;
    IAdapter* _adapter = nullptr;
    IInstance* _instance = nullptr;

    uint64_t _sizeInBytes = 0;
    uint32_t _frameCount = 0;
    uint8_t* _buffer = nullptr;

    OutputBufferSignalData& _signalData;

public:
    CoreAudioOutputBuffer(IDevice* device, uint64_t sizeInBytes, uint32_t frameCount, uint8_t* buffer, OutputBufferSignalData& signalData);
    ~CoreAudioOutputBuffer();

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

class CoreAudioInputBuffer : virtual public IInputBuffer, virtual public CollectedByHeap {
private:
    IDevice* _device = nullptr;
    IAdapter* _adapter = nullptr;
    IInstance* _instance = nullptr;

    uint64_t _sizeInBytes = 0;
    uint32_t _frameCount = 0;
    bool _empty = false;
    bool _silent = true;
    bool _discontinuity = false;
    uint64_t _timestamp = 0;

    uint8_t* _buffer = nullptr;
    bool _consumed = false;

public:
    CoreAudioInputBuffer(IDevice* device, uint64_t sizeInBytes, uint32_t frameCount, bool empty, bool silent, bool discontinuity, uint64_t timestamp);
    ~CoreAudioInputBuffer();

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
