#pragma once

#include <klatr/audio/audio.hpp>
#include <klatr/audio/instance.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

class CoreAudioInstance : virtual public IInstance, virtual public CollectedByHeap, virtual public ParentByVector {
private:


public:

};

}

}

}

#endif

namespace klatr {

namespace audio {

namespace coreaudio {

IInstance* createInstance() noexcept;

}

}

}
