#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/adapter.hpp>

#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudio.h>

namespace klatr {

namespace audio {

namespace internal {

namespace coreaudio {

class CoreAudioAdapter : virtual public IAdapter {
private:


public:

};

}

}

}

}

#endif
