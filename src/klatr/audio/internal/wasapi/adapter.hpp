#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/adapter.hpp>

#include <mmdeviceapi.h>
#include <audioclient.h>

namespace klatr {

namespace audio {

namespace internal {

namespace wasapi {

class WASAPIAdapter : virtual public IAdapter {
private:


public:

};

}

}

}

}

#endif
