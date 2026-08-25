#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/device.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

class WASAPIDevice : virtual public IDevice, virtual public CollectedByHeap, virtual public ParentByVector {
private:


public:

};

}

}

}

#endif
