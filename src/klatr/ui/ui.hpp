#pragma once

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

namespace klatr {

namespace ui {

enum class UICoordinateSpace : uint32_t {
    World = 0,
    Canvas = 1,
    Pixel = 2,
};

struct UIVector {
    UICoordinateSpace space;
    float x;
    float y;
    float z;
};

class IUIElement : virtual public IBase {
public:
    static inline IID const& iid() noexcept {
        static IID iid = IID("dfc9f46e-3dbf-4e6b-8a6e-fee831befbf3");
        return iid;
    }
};

class IUIHierarchical : virtual public IUIElement, virtual public IParent, virtual public IChild {
public:
    static inline IID const& iid() noexcept {
        static IID iid = IID("febfec0c-6dcd-4fff-9ba1-62872ab23095");
        return iid;
    }
};

class IUIPositional : virtual public IUIElement {
public:
    virtual void getPosition(UIVector* position) const noexcept = 0;
    virtual void setPosition(UIVector const* position) noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("94390e99-808d-499f-acef-3a5e5f2aae87");
        return iid;
    }
};

class IUIScalable : virtual public IUIElement {
public:
    virtual void getScale(UIVector* scale) const noexcept = 0;
    virtual void setScale(UIVector const* scale) noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("42931659-51e8-4bad-904f-688bf50b9d28");
        return iid;
    }
};

class IUIRotatable : virtual public IUIElement {
public:
    virtual float getRotation() const noexcept = 0;
    virtual void setRotation(float radians) noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("715fbcb8-dfd2-4a5b-a447-0f5f3de4370e");
        return iid;
    }
};

class IUIPointCollider : virtual public IUIElement {
public:
    virtual bool pointInside(UIVector const* point) const noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("547a8362-542a-4ab2-be4d-5187eedc68b8");
        return iid;
    }
};

class IUIReferenceList : virtual public IUIElement {
public:
    virtual bool adoptReference(kom::UUID const* typeUUID, kom::UUID const* valueUUID) noexcept = 0;
    virtual void disownReference(kom::UUID const* typeUUID, kom::UUID const* valueUUID) noexcept = 0;

    virtual bool enumerateReference(uint32_t id, kom::UUID* typeUUID, kom::UUID* valueUUID) const noexcept = 0;
    virtual bool enumerateReferenceValues(uint32_t id, kom::UUID const* typeFilter, kom::UUID* valueUUID) const noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("bf48cad0-c87d-4794-b02f-2f9906bc129a");
        return iid;
    }
};

class IUIRectangle : virtual public IUIPositional, virtual public IUIRotatable, virtual public IUIScalable, virtual public IUIPointCollider {
public:
    static inline IID const& iid() noexcept {
        static IID iid = IID("e3f030bb-6b76-449a-b690-86a4b920e47e");
        return iid;
    }
};

class IUICoordinateSpaceConverter : virtual public IUIElement {
public:
    virtual bool convertUIVector(UICoordinateSpace destinationSpace, UIVector* destination, UIVector const* source) const noexcept = 0;

    static inline IID const& iid() noexcept {
        static IID iid = IID("24588a0b-d6d2-436e-84ca-8384df271308");
        return iid;
    }
};

class UIContext : virtual public IUIElement, virtual public ParentByVector {
private:
    float _canvasWidth = 20.0f;
    float _canvasHeight = 20.0f;

    float _worldZoom = 1.0f;
    float _worldOffsetX = 0.0f;
    float _worldOffsetY = 0.0f;

public:
    UIContext(float canvasWidth = 20.0f, float canvasHeight = 20.0f);
    ~UIContext();

    float getCanvasWidth() const noexcept;
    float getCanvasHeight() const noexcept;

    float getWorldZoom() const noexcept;
    float getWorldOffsetX() const noexcept;
    float getWorldOffsetY() const noexcept;
    void setWorldZoom(float zoom) noexcept;
    void setWorldOffsetX(float x) noexcept;
    void setWorldOffsetY(float y) noexcept;

    static inline IID const& iid() noexcept {
        static IID iid = IID("1b68ad5a-4374-4a07-b9f8-4143f9754a19");
        return iid;
    }
};

}

}
