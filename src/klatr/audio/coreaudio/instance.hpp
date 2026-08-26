#pragma once

#include <klatr/audio/audio.hpp>
#include <klatr/audio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/coreaudio/adapter.hpp>

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

class CoreAudioInstance : virtual public IInstance, virtual public CollectedByHeap, virtual public ParentByVector {
private:
    std::vector<CoreAudioAdapter*> _adapters = {};

public:
    CoreAudioInstance();
    ~CoreAudioInstance();

    /* IInstance */
    InstanceBackendFlags backend() const noexcept override;
    IAdapter* enumerateAdapters(uint32_t id, IID const& filter) const noexcept override;
    IAdapter* defaultAdapter(DeviceFlowFlags flow) const noexcept override;

    /* IInterface */
    void* queryInterface(IID const& iid) noexcept override;

};

}

}

}

#endif

namespace klatr {

namespace audio {

namespace coreaudio {

IInstance* createInstance() noexcept;

}

}

}
