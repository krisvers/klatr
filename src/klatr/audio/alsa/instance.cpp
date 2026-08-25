#include <klatr/audio/alsa/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_ALSA

#include <klatr/audio/alsa/alsa.hpp>

namespace klatr {

namespace audio {

namespace alsa {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

#else

namespace klatr {

namespace audio {

namespace alsa {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

#endif
