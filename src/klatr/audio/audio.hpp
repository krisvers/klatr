#pragma once

#include <klatr/object.hpp>
#include <klatr/platform.hpp>

#ifdef KLATR_PLATFORM_FAMILY_NT
#ifndef KLATR_AUDIO_BACKEND_NO_WASAPI
#define KLATR_AUDIO_BACKEND_WASAPI
#endif
#elif defined(KLATR_PLATFORM_FAMILY_APPLE)
#ifndef KLATR_AUDIO_BACKEND_NO_COREAUDIO
#define KLATR_AUDIO_BACKEND_COREAUDIO
#endif
#elif defined(KLATR_PLATFORM_OS_LINUX)
#ifndef KLATR_AUDIO_BACKEND_NO_ALSA
#define KLATR_AUDIO_BACKEND_ALSA
#endif
#else
#error "Unsupported platform currently"
#endif

namespace klatr {

namespace audio {

class IAudioElement : virtual public IBase {
public:
    static inline IID const& iid() noexcept {
        static IID iid = IID("4216c034-948c-48fa-9f71-66e61b87cf60");
        return iid;
    }
};

}

}
