#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/device.hpp>

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

class CoreAudioDevice : virtual public IDevice, virtual public CollectedByHeap, virtual public ParentByVector {
private:


public:

};

}

}

}

#endif
