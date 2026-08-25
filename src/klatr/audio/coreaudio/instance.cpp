#include <klatr/audio/coreaudio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

#else

namespace klatr {

namespace audio {

namespace coreaudio {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

#endif
