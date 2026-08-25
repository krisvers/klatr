#pragma once

#include <klatr/audio/audio.hpp>
#include <klatr/audio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>

namespace klatr {

namespace audio {

namespace internal {

namespace coreaudio {

class CoreAudioInstance : virtual public IInstance {
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

namespace coreaudio {

IInstance* createInstance() noexcept;

}

}

}

}
