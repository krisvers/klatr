#pragma once

#include <vector>

#include <kom/kom.hpp>

namespace klatr {

using IInterface = kom::IInterface;
using IID = kom::IID;

/* adapted from krisvers/vkom */
class IBase : virtual public IInterface {
public:
    static inline IID const& iid() noexcept {
        static IID iid = IID("2ed7e7ab-a707-46f4-9aab-b909284ae2a0");
        return iid;
    }
};

class ICollected : virtual public IBase {
public:
    virtual uint32_t retain() = 0;
    virtual uint32_t release() = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("7cb3df2e-c1ac-46cc-ae37-230028240c71");
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
    inline T* enumerateChildren(uint32_t id) const noexcept {
        return enumerateChildren(id, T::iid())->template queryInterface<T>();
    }

    static inline IID const& iid() noexcept {
        static IID iid = IID("cc89508d-31ab-407f-afe0-65cd46153adb");
        return iid;
    }
};

class IChild : virtual public IBase {
public:
    virtual IParent* parent() const noexcept = 0;

    template<typename T>
    inline T* parent() const noexcept {
        return parent()->queryInterface<T>();
    }

    static inline IID const& iid() noexcept {
        static IID iid = IID("f34e169f-0084-49a4-9835-17282be77424");
        return iid;
    }
};

class CollectedByHeap : virtual public ICollected {
private:
    uint32_t _referenceCount = 0;

public:
    CollectedByHeap() = default;
    virtual ~CollectedByHeap() = 0;

    uint32_t release() override;
    uint32_t retain() override;

protected:
    uint32_t referenceCount() const noexcept;
};

class ParentByVector : virtual public IParent {
private:
    std::vector<IChild*> _children = {};

public:
    ParentByVector() = default;
    virtual ~ParentByVector() = 0;

    bool hasChild(IChild const* child) const noexcept override;
    IChild* enumerateChildren(uint32_t id, IID const& filter) const noexcept override;
    bool adopt(IChild* child) noexcept override;
    bool disown(IChild* child) noexcept override;

protected:
    void disownAll() noexcept;
};

}
