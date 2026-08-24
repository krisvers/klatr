#pragma once

#include <klatr/audio/audio.hpp>
#include <klatr/audio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <audioclient.h>

namespace klatr {

namespace audio {

namespace internal {

namespace wasapi {

class WASAPIInstance : virtual public IInstance {
private:


public:

};

}

}

}

}

#endif

namespace klatr {

namespace audio {

namespace internal {

namespace wasapi {

IInstance* createInstance() noexcept;

}

}

}

}
