#pragma once

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

#include <klatr/audio/audio.hpp>
#include <klatr/audio/enums.hpp>

namespace klatr {

namespace audio {

class IInstance : virtual public IAudioElement, virtual public IParent {
public:
    virtual InstanceBackendFlags backend() const noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("715dab3c-124f-4374-b891-880171e85c97");
        return iid;
    }
};

KLATR_VISIBLE InstanceBackendFlags querySupportedInstanceBackends() noexcept;
KLATR_VISIBLE IInstance* createInstance(InstanceBackendFlags backends) noexcept;

}

}
