#include <klatr/audio/internal/alsa/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_ALSA

namespace klatr {

namespace audio {

namespace internal {

namespace alsa {



}

}

}

}

#else

namespace klatr {

namespace audio {

namespace internal {

namespace alsa {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

}

#endif
