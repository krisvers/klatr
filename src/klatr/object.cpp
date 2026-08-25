#include <klatr/object.hpp>

namespace klatr {

/* adapted from krisvers/vkom */
CollectedByHeap::~CollectedByHeap() = default;

uint32_t CollectedByHeap::release() {
    if (_referenceCount == 0) {
        /* very likely currently destructing */
        return 0;
    }

    _referenceCount -= 1;
    if (_referenceCount == 0) {
        delete this;
        return 0;
    }

    return _referenceCount;
}

uint32_t CollectedByHeap::retain() {
    _referenceCount += 1;
    return _referenceCount;
}

uint32_t CollectedByHeap::referenceCount() const noexcept {
    return _referenceCount;
}

ParentByVector::~ParentByVector() = default;

bool ParentByVector::hasChild(IChild const* child) const noexcept {
    if (child == nullptr) {
        return false;
    }

    for (IChild const* c : _children) {
        if (c == child) {
            return true;
        }
    }

    return false;
}

IChild* ParentByVector::enumerateChildren(uint32_t id, IID const& filter) const noexcept {
    if (id >= _children.size()) {
        return nullptr;
    }

    if (filter == IID::null() || filter == IChild::iid() || filter == IBase::iid()) {
        IChild* child = _children[id];
        ICollected* collected = child->queryInterface<ICollected>();
        if (collected != nullptr) {
            collected->retain();
        }

        return child;
    }

    uint32_t currentID = 0;
    for (IChild* child : _children) {
        if (child->queryInterface(filter) != nullptr) {
            if (currentID == id) {
                ICollected* collected = child->queryInterface<ICollected>();
                if (collected != nullptr) {
                    collected->retain();
                }

                return child;
            }

            currentID += 1;
            if (currentID > id) {
                return nullptr;
            }
        }
    }

    return nullptr;
}

bool ParentByVector::adopt(IChild* child) noexcept {
    if (child == nullptr) {
        return false;
    }

    if (child->parent() != this) {
        return false;
    }

    if (hasChild(child)) {
        return false;
    }

    _children.push_back(child);
    ICollected* collected = child->queryInterface<ICollected>();
    if (collected != nullptr) {
        collected->retain();
    }

    return true;
}

bool ParentByVector::disown(IChild* child) noexcept {
    if (child == nullptr) {
        return false;
    }

    if (child->parent() != this) {
        return false;
    }

    for (auto it = _children.begin(); it != _children.end(); ++it) {
        if (*it == child) {
            _children.erase(it);

            ICollected* collected = child->queryInterface<ICollected>();
            if (collected != nullptr) {
                collected->release();
                return true;
            }

            return true;
        }
    }

    return false;
}

void ParentByVector::disownAll() noexcept {
    while (disown(enumerateChildren(0, IID::null()))) {}
}

}