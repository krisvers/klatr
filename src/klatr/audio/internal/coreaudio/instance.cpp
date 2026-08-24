#include <klatr/audio/internal/coreaudio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

namespace klatr {

namespace audio {

namespace internal {

namespace coreaudio {



}

}

}

}

#else

namespace klatr {

namespace audio {

namespace internal {

namespace coreaudio {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

}

#endif
