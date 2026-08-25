#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_ALSA

#include <klatr/audio/adapter.hpp>

#include <alsa/asoundlib.h>

namespace klatr {

namespace audio {

namespace internal {

namespace alsa {

class ALSAAdapter : virtual public IAdapter {
private:


public:

};

}

}

}

}

#endif
