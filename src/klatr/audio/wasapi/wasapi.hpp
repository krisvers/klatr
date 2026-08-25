#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <initguid.h>
#include <combaseapi.h>
#include <ks.h>
#include <ksmedia.h>
#include <stringapiset.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>

#endif
