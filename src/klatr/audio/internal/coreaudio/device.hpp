#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/device.hpp>

#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudio.h>

namespace klatr {

namespace audio {

namespace internal {

namespace coreaudio {

class CoreAudioDevice : virtual public IDevice {
private:


public:

};

}

}

}

}

#endif
