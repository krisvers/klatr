#include <klatr/audio/instance.hpp>
#include <klatr/audio/adapter.hpp>

#include <cassert>

int main(int argc, char** argv) {
    klatr::audio::IInstance* instance = klatr::audio::createInstance(klatr::audio::InstanceBackendFlags::Any);
    assert(instance != nullptr);

    klatr::audio::IAdapter* adapter = instance->enumerateAdapters(0);
    assert(adapter != nullptr);

    klatr::audio::AdapterInfo adapterInfo;
    adapter->getInfo(&adapterInfo);

    //

    instance->release();
    return 0;
}
