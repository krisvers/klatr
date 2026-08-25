#pragma once

#include <klatr/audio/audio.hpp>
#include <klatr/audio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_ALSA

#include <klatr/audio/alsa/alsa.hpp>

namespace klatr {

namespace audio {

namespace alsa {

class ALSAInstance : virtual public IInstance, virtual public CollectedByHeap, virtual public ParentByVector {
private:


public:

};

}

}

}

#endif

namespace klatr {

namespace audio {

namespace alsa {

IInstance* createInstance() noexcept;

}

}

}
