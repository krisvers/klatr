#pragma once

#include <kom/kom.hpp>

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

#include <klatr/audio/audio.hpp>
#include <klatr/audio/enums.hpp>

namespace klatr {

namespace audio {

class IBuffer : virtual public IAudioElement, virtual public ICollected, virtual public IChild {
public:
    virtual uint64_t sizeInBytes() const noexcept = 0;
    virtual uint32_t frameCount()  const noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("ed959433-be29-4ae9-9a3a-fc8c1b49b88b");
        return iid;
    }
};

class IOutputBuffer : virtual public IBuffer {
public:
    virtual void* map() noexcept = 0;
    virtual void unmap() noexcept = 0;
    virtual void silence() noexcept = 0;
    virtual void produce(uint32_t frameCount) noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("541bebd4-76cb-4b73-9acc-8ae26fb898c6");
        return iid;
    }
};

class IInputBuffer : virtual public IBuffer {
public:
    virtual void* map() noexcept = 0;
    virtual void unmap() noexcept = 0;
    virtual void consume() noexcept = 0;

    virtual bool empty() const noexcept = 0;
    virtual bool silent() const noexcept = 0;
    virtual bool discontinuity() const noexcept = 0;
    virtual uint64_t timestamp() const noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("7f8f487a-322b-4365-baf1-3b0998052093");
        return iid;
    }
};

}

}
