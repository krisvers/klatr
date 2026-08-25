#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_ALSA

#include <klatr/audio/adapter.hpp>

#include <klatr/audio/alsa/alsa.hpp>

namespace klatr {

namespace audio {

namespace alsa {

class ALSAAdapter : virtual public IAdapter, virtual public ParentByVector {
private:


public:

};

}

}

}

#endif
