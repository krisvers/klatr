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

enum class DeviceChannelFlags : uint32_t {
    None = 0x00000000,
    Left = 0x00000001,
    Right = 0x00000002,
    Center = 0x00000004,
    Subwoofer = 0x00000008,
    LeftSurround = 0x00000010,
    RightSurround = 0x00000020,
    CenterSurround = 0x00000040,
    LeftCenter = 0x00000080,
    RightCenter = 0x00000100,
    LeftSideSurround = 0x00000200,
    RightSideSurround = 0x00000400,
    LeftCenterTop = 0x00000800,
    RightCenterTop = 0x00001000,
    FrontCenterTop = 0x00002000,
    MidCenterTop = 0x00004000,
    LeftSurroundTop = 0x00008000,
    RightSurroundTop = 0x00010000,
    CenterSurroundTop = 0x00020000,

    Mono = Center,
    Stereo = Left | Right,
    DualStereo = Left | Right | LeftSurround | RightSurround,
    Surround4 = Left | Right | Center | CenterSurround,
    Surround51 = Left | Right | Center | Subwoofer | LeftSideSurround | RightSideSurround,
    RearSurround51 = Left | Right | Center | Subwoofer | LeftSurround | RightSurround,
    Surround71 = Left | Right | Center | Subwoofer | LeftSurround | RightSurround | LeftSideSurround | RightSideSurround,
};

KLATR_DEFINE_ENUM_BITFLAGS_OPERATORS(DeviceChannelFlags)

enum class DeviceTransportType : uint32_t {
    Unknown = 0,
    Integrated = 1,
    DisplayCable = 2,
    DedicatedSoundCard = 3,
    Network = 4,
    Bluetooth = 5,
    Other = 6,
};

enum class DeviceFlowFlags : uint32_t {
    None = 0x0000,
    Output = 0x0001,
    Input = 0x0002,
};

KLATR_DEFINE_ENUM_BITFLAGS_OPERATORS(DeviceFlowFlags)

enum class FormatFlags : uint32_t {
    None = 0x0000,
    Sint16 = 0x0001,
    Sint24 = 0x0002,
    Sint32 = 0x0004,
    Float32 = 0x0008,
};

KLATR_DEFINE_ENUM_BITFLAGS_OPERATORS(FormatFlags)

}

}
