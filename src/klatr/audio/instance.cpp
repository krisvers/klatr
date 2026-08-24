#include <klatr/audio/instance.hpp>

#include <klatr/audio/audio.hpp>
#include <klatr/audio/enums.hpp>

#include <klatr/audio/internal/wasapi/instance.hpp>
#include <klatr/audio/internal/coreaudio/instance.hpp>
#include <klatr/audio/internal/alsa/instance.hpp>

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
        IInstance* instance = internal::wasapi::createInstance();
        if (instance != nullptr) {
            return instance;
        }
    }

    if ((backends & InstanceBackendFlags::CoreAudio) != InstanceBackendFlags::None) {
        IInstance* instance = internal::coreaudio::createInstance();
        if (instance != nullptr) {
            return instance;
        }
    }

    if ((backends & InstanceBackendFlags::ALSA) != InstanceBackendFlags::None) {
        IInstance* instance = internal::alsa::createInstance();
        if (instance != nullptr) {
            return instance;
        }
    }

    return nullptr;
}

}

}
