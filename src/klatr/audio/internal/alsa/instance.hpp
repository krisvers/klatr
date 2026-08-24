#pragma once

#include <klatr/audio/audio.hpp>
#include <klatr/audio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_ALSA

#include <alsa/asoundlib.h>

namespace klatr {

namespace audio {

namespace internal {

namespace alsa {

class ALSAInstance : virtual public IInstance {
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

namespace alsa {

IInstance* createInstance() noexcept;

}

}

}

}
