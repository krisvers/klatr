#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <CoreFoundation/CoreFoundation.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>

struct CoreAudioDeviceTransportType {
    UInt32 type;
};

#endif
