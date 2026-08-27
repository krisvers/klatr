#include <klatr/ui/ui.hpp>

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

namespace klatr {

namespace ui {

UIContext::UIContext(float canvasWidth, float canvasHeight) : _canvasWidth(canvasWidth), _canvasHeight(canvasHeight) {

}

UIContext::~UIContext() {
    ParentByVector::disownAll();
}

}

}
