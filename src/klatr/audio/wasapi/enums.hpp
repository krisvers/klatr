#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_WASAPI

#include <klatr/audio/enums.hpp>

#include <klatr/audio/wasapi/wasapi.hpp>

namespace klatr {

namespace audio {

namespace wasapi {

inline DeviceChannelFlags castEnum(KSAUDIO_CHANNEL_CONFIG config) {
    DeviceChannelFlags flags = DeviceChannelFlags::None;
    if ((config.ActiveSpeakerPositions & SPEAKER_FRONT_LEFT) != 0) {
        flags |= DeviceChannelFlags::Left;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_FRONT_RIGHT) != 0) {
        flags |= DeviceChannelFlags::Right;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_FRONT_CENTER) != 0) {
        flags |= DeviceChannelFlags::Center;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_LOW_FREQUENCY) != 0) {
        flags |= DeviceChannelFlags::Subwoofer;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_BACK_LEFT) != 0) {
        flags |= DeviceChannelFlags::LeftSurround;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_BACK_RIGHT) != 0) {
        flags |= DeviceChannelFlags::RightSurround;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_FRONT_LEFT_OF_CENTER) != 0) {
        flags |= DeviceChannelFlags::LeftCenter;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_FRONT_RIGHT_OF_CENTER) != 0) {
        flags |= DeviceChannelFlags::RightCenter;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_BACK_CENTER) != 0) {
        flags |= DeviceChannelFlags::CenterSurround;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_SIDE_LEFT) != 0) {
        flags |= DeviceChannelFlags::LeftSideSurround;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_SIDE_RIGHT) != 0) {
        flags |= DeviceChannelFlags::RightSideSurround;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_TOP_CENTER) != 0) {
        flags |= DeviceChannelFlags::MidCenterTop;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_TOP_FRONT_LEFT) != 0) {
        flags |= DeviceChannelFlags::LeftCenterTop;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_TOP_FRONT_CENTER) != 0) {
        flags |= DeviceChannelFlags::FrontCenterTop;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_TOP_FRONT_RIGHT) != 0) {
        flags |= DeviceChannelFlags::RightCenterTop;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_TOP_BACK_LEFT) != 0) {
        flags |= DeviceChannelFlags::LeftSurroundTop;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_TOP_BACK_CENTER) != 0) {
        flags |= DeviceChannelFlags::CenterSurroundTop;
    }

    if ((config.ActiveSpeakerPositions & SPEAKER_TOP_BACK_RIGHT) != 0) {
        flags |= DeviceChannelFlags::RightSurroundTop;
    }

    return flags;
}

inline KSAUDIO_CHANNEL_CONFIG castEnum(DeviceChannelFlags flags) {
    KSAUDIO_CHANNEL_CONFIG config = {};
    if ((flags & DeviceChannelFlags::Left) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_FRONT_LEFT;
    }

    if ((flags & DeviceChannelFlags::Right) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_FRONT_RIGHT;
    }

    if ((flags & DeviceChannelFlags::Center) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_FRONT_CENTER;
    }

    if ((flags & DeviceChannelFlags::Subwoofer) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_LOW_FREQUENCY;
    }

    if ((flags & DeviceChannelFlags::LeftSurround) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_BACK_LEFT;
    }

    if ((flags & DeviceChannelFlags::RightSurround) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_BACK_RIGHT;
    }

    if ((flags & DeviceChannelFlags::LeftCenter) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_FRONT_LEFT_OF_CENTER;
    }

    if ((flags & DeviceChannelFlags::RightCenter) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_FRONT_RIGHT_OF_CENTER;
    }

    if ((flags & DeviceChannelFlags::CenterSurround) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_BACK_CENTER;
    }

    if ((flags & DeviceChannelFlags::LeftSideSurround) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_SIDE_LEFT;
    }

    if ((flags & DeviceChannelFlags::RightSideSurround) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_SIDE_RIGHT;
    }

    if ((flags & DeviceChannelFlags::MidCenterTop) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_TOP_CENTER;
    }

    if ((flags & DeviceChannelFlags::LeftCenterTop) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_TOP_FRONT_LEFT;
    }

    if ((flags & DeviceChannelFlags::FrontCenterTop) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_TOP_FRONT_CENTER;
    }

    if ((flags & DeviceChannelFlags::RightCenterTop) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_TOP_FRONT_RIGHT;
    }

    if ((flags & DeviceChannelFlags::LeftSurroundTop) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_TOP_BACK_LEFT;
    }

    if ((flags & DeviceChannelFlags::CenterSurroundTop) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_TOP_BACK_CENTER;
    }

    if ((flags & DeviceChannelFlags::RightSurroundTop) != DeviceChannelFlags::None) {
        config.ActiveSpeakerPositions |= SPEAKER_TOP_BACK_RIGHT;
    }

    return config;
}

inline DeviceTransportType castEnum(EndpointFormFactor form) {
    switch (form) {
        case RemoteNetworkDevice:
            return DeviceTransportType::Network;
        case Speakers:
        case Headphones:
        case Microphone:
            return DeviceTransportType::Integrated;
        case LineLevel:
        case SPDIF:
            return DeviceTransportType::DedicatedSoundCard;
        case DigitalAudioDisplayDevice:
            return DeviceTransportType::DisplayCable;
        case Headset:
        case Handset:
        case UnknownDigitalPassthrough:
        case UnknownFormFactor:
            return DeviceTransportType::Other;
        default:
            break;
    }

    return DeviceTransportType::Other;
}

inline DeviceFlowFlags castEnum(EDataFlow flow) {
    switch (flow) {
        case eRender:
            return DeviceFlowFlags::Output;
        case eCapture:
            return DeviceFlowFlags::Input;
        case eAll:
            return DeviceFlowFlags::Output | DeviceFlowFlags::Input;
        default:
            break;
    }

    return DeviceFlowFlags::None;
}

inline EDataFlow castEnum(DeviceFlowFlags flags) {
    if (flags == (DeviceFlowFlags::Output | DeviceFlowFlags::Input)) {
        return eAll;
    } else if (flags == DeviceFlowFlags::Output) {
        return eRender;
    } else if (flags == DeviceFlowFlags::Input) {
        return eCapture;
    }

    return eAll;
}

}

}

}

#endif
