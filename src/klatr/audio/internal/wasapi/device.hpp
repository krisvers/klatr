#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/device.hpp>

#include <mmdeviceapi.h>
#include <audioclient.h>

namespace klatr {

namespace audio {

namespace internal {

namespace wasapi {

class WASAPIDevice : virtual public IDevice {
private:


public:

};

}

}

}

}

#endif
