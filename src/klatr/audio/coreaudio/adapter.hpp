#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/adapter.hpp>

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

class CoreAudioAdapter : virtual public IAdapter, virtual public ParentByVector {
private:


public:

};

}

}

}

#endif
