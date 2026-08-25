#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_ALSA

#include <klatr/audio/device.hpp>

#include <alsa/asoundlib.h>

namespace klatr {

namespace audio {

namespace internal {

namespace alsa {

class ALSADevice : virtual public IDevice {
private:


public:

};

}

}

}

}

#endif
