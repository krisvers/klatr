#include <klatr/audio/internal/wasapi/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

namespace klatr {

namespace audio {

namespace internal {

namespace wasapi {



}

}

}

}

#else

namespace klatr {

namespace audio {

namespace internal {

namespace wasapi {

IInstance* createInstance() noexcept {
    return nullptr;
}

}

}

}

}

#endif
