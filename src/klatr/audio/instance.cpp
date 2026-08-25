#include <klatr/audio/instance.hpp>

#include <klatr/audio/audio.hpp>
#include <klatr/audio/enums.hpp>

#include <klatr/audio/wasapi/instance.hpp>
#include <klatr/audio/coreaudio/instance.hpp>
#include <klatr/audio/alsa/instance.hpp>

namespace klatr {

namespace audio {

InstanceBackendFlags querySupportedInstanceBackends() noexcept {
    InstanceBackendFlags flags = InstanceBackendFlags::None;
    #ifdef KLATR_AUDIO_BACKEND_WASAPI
    flags |= InstanceBackendFlags::WASAPI;
    #endif

    #ifdef KLATR_AUDIO_BACKEND_COREAUDIO
    flags |= InstanceBackendFlags::CoreAudio;
    #endif

    #ifdef KLATR_AUDIO_BACKEND_ALSA
    flags |= InstanceBackendFlags::ALSA;
    #endif

    /* TODO: */

    return flags;
}

IInstance* createInstance(InstanceBackendFlags backends) noexcept {
    if ((backends & InstanceBackendFlags::WASAPI) != InstanceBackendFlags::None) {
        IInstance* instance = wasapi::createInstance();
        if (instance != nullptr) {
            return instance;
        }
    }

    if ((backends & InstanceBackendFlags::CoreAudio) != InstanceBackendFlags::None) {
        IInstance* instance = coreaudio::createInstance();
        if (instance != nullptr) {
            return instance;
        }
    }

    if ((backends & InstanceBackendFlags::ALSA) != InstanceBackendFlags::None) {
        IInstance* instance = alsa::createInstance();
        if (instance != nullptr) {
            return instance;
        }
    }

    return nullptr;
}

}

}
