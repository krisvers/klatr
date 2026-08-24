#pragma once

#include <klatr/enums.hpp>

namespace klatr {

namespace audio {

enum class InstanceBackendFlags : uint32_t {
    None = 0x0000,

    /* preferred */
    WASAPI = 0x0001,
    CoreAudio = 0x0002,
    ALSA = 0x0004,

    /* fallback */
    DirectSound = 0x0008,
    OSS = 0x0010,
};

KLATR_DEFINE_ENUM_BITFLAGS_OPERATORS(InstanceBackendFlags)

}

}
