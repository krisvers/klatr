#pragma once

#include <klatr/audio/audio.hpp>

#ifdef KLATR_AUDIO_BACKEND_COREAUDIO

#include <klatr/audio/enums.hpp>

#include <klatr/audio/coreaudio/coreaudio.hpp>

namespace klatr {

namespace audio {

namespace coreaudio {

inline DeviceChannelFlags castEnum(AudioChannelBitmap bitmap) {
    DeviceChannelFlags flags = DeviceChannelFlags::None;
    if ((bitmap & kAudioChannelBit_Left) != 0) {
        flags |= DeviceChannelFlags::Left;
    }

    if ((bitmap & kAudioChannelBit_Right) != 0) {
        flags |= DeviceChannelFlags::Right;
    }

    if ((bitmap & kAudioChannelBit_Center) != 0) {
        flags |= DeviceChannelFlags::Center;
    }

    if ((bitmap & kAudioChannelBit_LFEScreen) != 0) {
        flags |= DeviceChannelFlags::Subwoofer;
    }

    if ((bitmap & kAudioChannelBit_LeftSurround) != 0) {
        flags |= DeviceChannelFlags::LeftSurround;
    }

    if ((bitmap & kAudioChannelBit_RightSurround) != 0) {
        flags |= DeviceChannelFlags::RightSurround;
    }

    if ((bitmap & kAudioChannelBit_LeftCenter) != 0) {
        flags |= DeviceChannelFlags::LeftCenter;
    }

    if ((bitmap & kAudioChannelBit_RightCenter) != 0) {
        flags |= DeviceChannelFlags::RightCenter;
    }

    if ((bitmap & kAudioChannelBit_CenterSurround) != 0) {
        flags |= DeviceChannelFlags::CenterSurround;
    }

    if ((bitmap & kAudioChannelBit_LeftSurroundDirect) != 0) {
        flags |= DeviceChannelFlags::LeftSideSurround;
    }

    if ((bitmap & kAudioChannelBit_RightSurroundDirect) != 0) {
        flags |= DeviceChannelFlags::RightSideSurround;
    }

    if ((bitmap & kAudioChannelBit_TopCenterSurround) != 0) {
        flags |= DeviceChannelFlags::CenterSurroundTop;
    }

    if ((bitmap & kAudioChannelBit_TopBackLeft) != 0) {
        flags |= DeviceChannelFlags::LeftSurroundTop;
    }

    if ((bitmap & kAudioChannelBit_TopBackRight) != 0) {
        flags |= DeviceChannelFlags::RightSurroundTop;
    }

    if ((bitmap & kAudioChannelBit_LeftTopFront) != 0) {
        flags |= DeviceChannelFlags::LeftCenterTop;
    }

    if ((bitmap & kAudioChannelBit_CenterTopFront) != 0) {
        flags |= DeviceChannelFlags::MidCenterTop;
    }

    if ((bitmap & kAudioChannelBit_RightTopFront) != 0) {
        flags |= DeviceChannelFlags::RightCenterTop;
    }

    /* TODO: this is probably really wrong */

    return flags;
}

inline DeviceTransportType castEnum(CoreAudioDeviceTransportType transport) {
    switch (transport.type) {
        case kAudioDeviceTransportTypeBuiltIn:
        case kAudioDeviceTransportTypeUSB:
        case kAudioDeviceTransportTypeThunderbolt:
        case kAudioDeviceTransportTypeFireWire:
            return DeviceTransportType::Integrated;
        case kAudioDeviceTransportTypePCI:
            return DeviceTransportType::DedicatedSoundCard;
        case kAudioDeviceTransportTypeBluetooth:
        case kAudioDeviceTransportTypeBluetoothLE:
            return DeviceTransportType::Bluetooth;
        case kAudioDeviceTransportTypeHDMI:
        case kAudioDeviceTransportTypeDisplayPort:
            return DeviceTransportType::DisplayCable;
        case kAudioDeviceTransportTypeAirPlay:
            return DeviceTransportType::MiscWireless;
        case kAudioDeviceTransportTypeAVB:
        return DeviceTransportType::Network;
        case kAudioDeviceTransportTypeAggregate:
        case kAudioDeviceTransportTypeVirtual:
        case kAudioDeviceTransportTypeContinuityCaptureWired:
        case kAudioDeviceTransportTypeContinuityCaptureWireless:
            return DeviceTransportType::Other;
        case kAudioDeviceTransportTypeUnknown:
        default:
            break;
    }

    return DeviceTransportType::Unknown;
}

}

}

}

#endif
