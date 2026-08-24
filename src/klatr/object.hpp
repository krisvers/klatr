#pragma once

#include <kom/kom.hpp>

namespace klatr {

using IInterface = kom::IInterface;
using IID = kom::IID;

/* adapted from krisvers/vkom */
class IBase : virtual public IInterface {
public:
    static inline IID const& iid() noexcept {
        static IID iid = IID("0c7b1d80-59bf-4da8-9f9f-e9620747b596");
        return iid;
    }
};

class ICollected : virtual public IBase {
public:
    virtual uint32_t retain() noexcept = 0;
    virtual uint32_t release() noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("d1ebd19c-1149-4868-8f0f-e6881b3f2232");
        return iid;
    }
};

class IChild;

class IParent : virtual public IBase {
public:
    virtual bool hasChild(IChild const* child) const noexcept = 0;
    virtual IChild* enumerateChildren(uint32_t id, IID const& filter = IID::null()) const noexcept = 0;
    virtual bool adopt(IChild* child) noexcept = 0;
    virtual bool disown(IChild* child) noexcept = 0;

    template<typename T>
    T* enumerateChildren(uint32_t id) const noexcept {
        return enumerateChildren(id, T::iid())->template queryInterface<T>();
    }

    static inline IID const& iid() noexcept {
        static IID iid = IID("a2efda53-8b1e-4d47-85f1-2789b915a03b");
        return iid;
    }
};

class IChild : virtual public IBase {
public:
    virtual IParent* parent() const noexcept = 0;

    template<typename T>
    T* parent() const noexcept {
        return parent()->queryInterface<T>();
    }

    static inline IID const& iid() noexcept {
        static IID iid = IID("589186c1-6270-4cea-a31f-f46be34292b4");
        return iid;
    }
};

}
