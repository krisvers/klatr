#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_ALSA

#include <klatr/audio/device.hpp>

#include <klatr/audio/alsa/alsa.hpp>

namespace klatr {

namespace audio {

namespace alsa {

class ALSADevice : virtual public IDevice, virtual public CollectedByHeap, virtual public ParentByVector {
private:


public:

};

}

}

}

#endif
