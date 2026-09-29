struct PushConstantAudioBufferDescriptor {
    uint64_t address;
    uint count;
    float2 bounds; /* note: if bounds.x == bounds.y this is treated as a constant */
};

struct PushConstants {
    PushConstantAudioBufferDescriptor buffer;
};

//struct UniformAudioBufferDescriptor {
//    uint count;
//    float2 bounds; /* note: if bounds.x == bounds.y this is treated as a constant */
//};

struct Uniforms {
    uint64_t globalID;
    float globalTime;
    uint dispatchWidth;
    uint sampleRate;
    float inverseSampleRate;
};

[[vk::push_constant]]
PushConstants pushConstants;

[[vk::binding(0, 0)]]
ConstantBuffer<Uniforms> uniforms;

[[vk::binding(0, 1)]]
[[vk::image_format("rgba8")]]
RWTexture2D<float4> tiles[1];

#define pi float(3.14159265358979323846f)

float lmap(float f, float a, float b, float x, float y) {
    float rangeAB = b - a;
    float rangeXY = y - x;
    
    return ((f + b) / rangeAB) * rangeXY + x;
}

bool rawReadSampleFromPushConstantBuffer(PushConstantAudioBufferDescriptor descriptor, uint64_t index, out float value, float defaultValue = 0.0, bool loopback = false) {
    value = defaultValue;
    if (index >= descriptor.count) {
        if (!loopback) {
            return false;
        }
        
        index %= descriptor.count;
    }
    
    value = vk::RawBufferLoad<float>(descriptor.address + index * sizeof(float));
    return true;
}

bool rawWriteSampleToPushConstantBuffer(PushConstantAudioBufferDescriptor descriptor, uint64_t index, float value, bool loopback = false) {
    if (index >= descriptor.count) {
        if (!loopback) {
            return false;
        }
        
        index %= descriptor.count;
    }
    
    vk::RawBufferStore<float>(descriptor.address + index * sizeof(float), value);
    return true;
}

bool readSampleFromPushConstantBuffer(PushConstantAudioBufferDescriptor descriptor, uint64_t index, out float value, float defaultValue = 0.0, bool loopback = false) {
    value = defaultValue;
    if (descriptor.bounds.x == descriptor.bounds.y) {
        value = descriptor.bounds.x;
        return true;
    }
    
    float rawValue;
    if (!rawReadSampleFromPushConstantBuffer(descriptor, index, rawValue, defaultValue, loopback)) {
        return false;
    }

    value = lmap(rawValue, -1.0, 1.0, descriptor.bounds.x, descriptor.bounds.y);
    return true;
}

bool writeSampleToPushConstantBuffer(PushConstantAudioBufferDescriptor descriptor, uint64_t index, float value, bool loopback = false) {
    float normalizedValue;
    if (descriptor.bounds.x == descriptor.bounds.y) {
        normalizedValue = descriptor.bounds.x;
    } else {
        normalizedValue = lmap(value, descriptor.bounds.x, descriptor.bounds.y, -1.0, 1.0);
    }
    
    return rawWriteSampleToPushConstantBuffer(descriptor, index, normalizedValue, loopback);
}

void rawWriteToTile(RWTexture2D<float4> tile, float2 xy, float4 value) {
    if (xy.x < 0.0 || xy.x >= 1.0 || xy.y < 0.0 || xy.y >= 1.0) {
        return;
    }
    
    tile[xy * float2(512.0, 512.0)] = value;
}

float ssin(float t) {
    return sin(2 * pi * t);
}

float secondsFromSamplePeriod(uint64_t id, float period, uint sampleRate = uniforms.sampleRate) {
    float scale = 1 << 16;
    
    uint64_t scaledPeriodInSamples = uint64_t(period * sampleRate * scale);
    float scaled = float((id * scale) % scaledPeriodInSamples);
    return scaled / float(scale) / float(sampleRate) / period;
}

float sampleSineWave(uint64_t id, float frequency, float phase = 0.0, uint sampleRate = uniforms.sampleRate) {
    return sin(2 * pi * secondsFromSamplePeriod(id, 1.0 / frequency, sampleRate) + phase);
}

float sampleSawWave(uint64_t id, float frequency, float phase = 0.0, uint sampleRate = uniforms.sampleRate) {
    return 2.0 * fmod(lmap(2 * pi * secondsFromSamplePeriod(id, 1.0 / frequency, sampleRate), 0.0, 1.0, 0.0, 1.0) + phase, 1.0) - 1.0;
}

float sampleSquareWave(uint64_t id, float frequency, float phase = 0.0, uint sampleRate = uniforms.sampleRate) {
    return sign(fmod(lmap(2 * pi * secondsFromSamplePeriod(id, 1.0 / frequency, sampleRate), 0.0, 2 * pi, 0.0, 1.0) + phase, 1.0) - 0.5);
}

float sampleFMSine(uint64_t id, float carrierFrequency, float modulationDepth, float modulationFrequency, float carrierPhase = 0.0, float modulationPhase = 0.0, uint sampleRate = uniforms.sampleRate) {
    const float phi = -pi / 2.0;
    float carrier = 2 * pi * secondsFromSamplePeriod(id, 1.0 / carrierFrequency, sampleRate) + carrierPhase;
    float modulation = modulationDepth * sin(2 * pi * secondsFromSamplePeriod(id, 1.0 / modulationFrequency, sampleRate) + modulationPhase + phi);
    
    return sin(carrier + modulation + phi);
}

float bqerp(float x, float curve) {
    float w = (curve + 1.0) / 2.0;
    float t = (-w + sqrt(w * w + x * (1.0 - 2.0 * w))) / (1.0 - 2.0 * w);
    float y = t * (2.0 - 2.0 * w + 2.0 * t * w - t);
    
    return x;
}

/*
*/

float2 adsrEncode(float start, float end, float attack, float decay, float release) {
    if (end > release) {
        return float2(2.0, 1.0);
    }
    
    float t = start - max(end, 0.0);
    
    float2 value = float2(0.0, 0.0);
    if (t < 0.0) {
        value.x = 0.0;
    } else if (t < attack) {
        value.x = lmap(t, 0.0, attack, 0.0, 1.0);
    } else if (start < attack + decay) {
        value.x = lmap(t, attack, attack + decay, 1.0, 2.0);
    } else {
        value.x = 2.0;
    }
    
    if (end > 0.0) {
        value.y = lmap(end, 0.0, release, 0.0, 1.0);
    }
    
    return value;
}

float adsrbqerpDecode(float2 encoded, float sustain, float attackCurve, float decayCurve, float releaseCurve) {
    float value = 0.0;
    if (encoded.x < 1.0) {
        value = bqerp(encoded.x, attackCurve);
    } else if (encoded.x < 2.0) {
        value = lmap(bqerp(encoded.x - 2.0, decayCurve), 0.0, 1.0, 1.0, sustain);
    } else if (encoded.x >= 2.0) {
        value = sustain;
    }
    
    if (encoded.y <= 0.0) {
        return value;
    }
    
    return lmap(bqerp(encoded.y, releaseCurve), 0.0, 1.0, 1.0, 0.0);
}

float adsrbqerp(float start, float end, float attack, float decay, float sustain, float release, float attackCurve = 0.0, float decayCurve = 0.0, float releaseCurve = 0.0) {
    return adsrbqerpDecode(adsrEncode(start, end, attack, decay, release), sustain, attackCurve, decayCurve, releaseCurve);
}

[numthreads(32, 32, 1)]
void module(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint3 threadCoordinateID : SV_DispatchThreadID) {
    const uint3 groupDimensions = uint3(32, 32, 1);
    const uint3 dispatchDimensions = uint3(uniforms.dispatchWidth, 1, 1);

    uint localID = groupThreadID.x + groupDimensions.x * (groupThreadID.y + groupDimensions.y * (groupThreadID.z + groupDimensions.z * (groupID.x + dispatchDimensions.x * (groupID.y + dispatchDimensions.y * groupID.z))));
    uint64_t id = localID + uniforms.globalID;
    float t = float(id) / float(uniforms.sampleRate);

    //float v = sign(fmod(angleFromSamplePeriod(id, uniforms.sampleRate, 1.0 / 220.0), 1.0) - 0.5);
    //float v = 2.0 * fmod(t * 220.0, 1.0) - 1.0;
    //float v = sampleSine(id, uniforms.sampleRate, sampleSine(id, uniforms.sampleRate, 1, 0.0, 110.0, 220.0));
    //float v = sin(2.0 * pi * t * 220.0);
    //vk::RawBufferStore<float>(pushConstants.address + id * 4, v);
    
    //float v = sampleSawWave(id, 220.0) * 0.2;
    float2 encoded = adsrEncode(t - 0.25, t - 1.75, 0.2, 0.2, 0.2);
    float v = adsrbqerpDecode(encoded, 0.25, 0.0, 0.0, 0.0) * sampleFMSine(id, 110.0, 1.0, 0.0) * 0.4;
    
    float4 color = float4(0.3, 0.3, 0.3, 1.0);
    if (encoded.x < 1.0) {
        color = float4(encoded.x, 0.0, 0.0, 1.0);
    } else if (encoded.x < 2.0) {
        color = float4(0.0, 0.0, encoded.x, 1.0);
    } else if (encoded.x == 2.0) {
        color = float4(1.0, 0.0, 1.0, 1.0);
    }
    
    color.y = encoded.y;
    
    rawWriteToTile(tiles[0], float2(float(localID) / float(32768.0 * 32), lmap(encoded.x, -10.0, 10.0, 0.5, 0.0)), color);

    //audioBuffer[localID] = v;
    writeSampleToPushConstantBuffer(pushConstants.buffer, localID, v);
}
