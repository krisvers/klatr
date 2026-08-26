#pragma once

#include <type_traits>

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

#include <klatr/audio/audio.hpp>
#include <klatr/audio/enums.hpp>
#include <klatr/audio/adapter.hpp>

namespace klatr {

namespace audio {

class IInstance : virtual public IAudioElement, virtual public ICollected, virtual public IParent {
public:
    virtual InstanceBackendFlags backend() const noexcept = 0;
    virtual IAdapter* enumerateAdapters(uint32_t id, IID const& filter = IAdapter::iid()) const noexcept = 0;
    virtual IAdapter* defaultAdapter(DeviceFlowFlags flow) const noexcept = 0;

    template<typename T>
    inline T* enumerateAdapters(uint32_t id) const noexcept {
        static_assert(std::is_base_of<IAdapter, T>::value, "T must inherit from IAdapter");
        return enumerateAdapters(id, T::iid())->template queryInterface<T>();
    }

    template<typename T>
    inline T* defaultAdapter(DeviceFlowFlags flow) const noexcept {
        static_assert(std::is_base_of<IAdapter, T>::value, "T must inherit from IAdapter");
        IAdapter* adapter = defaultAdapter(flow);
        if (adapter == nullptr) {
            return nullptr;
        }

        return adapter->queryInterface<T>();
    }

    static inline IID const& iid() noexcept {
        static IID iid = IID("715dab3c-124f-4374-b891-880171e85c97");
        return iid;
    }
};

KLATR_VISIBLE InstanceBackendFlags querySupportedInstanceBackends() noexcept;
KLATR_VISIBLE IInstance* createInstance(InstanceBackendFlags backends = InstanceBackendFlags::Any) noexcept;

}

}
