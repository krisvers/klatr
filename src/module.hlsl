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

class IInterpolator {
    /* maps t within [0, 1] -> [0, 1] but with a certain curvature */
    float interpolate(float x) {
        return x;
    }
};

class LinearInterpolator : IInterpolator{
    float interpolate(float x) {
        return lerp(0.0, 1.0, x);
    }
};

class BezierQuadInterpolator : IInterpolator {
    float _curve;

    BezierQuadInterpolator interpolator(float curve) {
        BezierQuadInterpolator i;
        i.setCurve(curve);
        return i;
    }

    void setCurve(float curve) {
        _curve = curve;
    }

    float interpolate(float x) {
        /* solving a parametric set of x(t), y(t) for bezier quadratic curve as y(x) */
        float w = (_curve + 1.0) / 2.0;
        float t = (-w + sqrt(w * w + x * (1.0 - 2.0 * w))) / (1.0 - 2.0 * w);
        float y = t * (2.0 - 2.0 * w + 2.0 * t * w - t);

        return y;
    }
};

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

    tile[xy * float2(511.0, 511.0)] = value;
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

float samplePitchEnvelopeSine(uint64_t id, float t, float startFrequency, float endFrequency, float decay, uint sampleRate = uniforms.sampleRate) {
    //float frequency = 2 * pi * (startFrequency + (endFrequency - startFrequency) / decay * t);
    float angle = 2 * pi * (startFrequency * t + 0.5 * (endFrequency - startFrequency) / decay * t * t);
    if (t < 0.0) {
        return 0.0;
    } else if (t > decay) {
        float phase = 2 * pi * (startFrequency * decay + 0.5 * (endFrequency - startFrequency) * decay);
        return sin(2 * pi * t * endFrequency + phase);
    }

    return sin(angle);
}

float sampleFMPitchEnvelopeSine(uint64_t id, float t, float startCarrierFrequency, float endCarrierFrequency, float carrierDecay, float modulationDepth, float modulationFrequency, float carrierPhase = 0.0, float modulationPhase = 0.0, uint sampleRate = uniforms.sampleRate) {
    float angle = 0.0;
    if (t > 0.0 && t < carrierDecay) {
        angle = 2 * pi * (startCarrierFrequency * t + 0.5 * (endCarrierFrequency - startCarrierFrequency) / carrierDecay * t * t);
    }

    const float phi = -pi / 2.0;
    float carrier = 2 * pi * secondsFromSamplePeriod(id, 1.0 / endCarrierFrequency, sampleRate) + carrierPhase;
    float modulation = modulationDepth * sin(2 * pi * secondsFromSamplePeriod(id, 1.0 / modulationFrequency, sampleRate) + modulationPhase + phi);

    return sin(carrier + modulation + angle + phi);
}

float bqerp(float x, float curve) {
    float w = (curve + 1.0) / 2.0;
    float t = (-w + sqrt(w * w + x * (1.0 - 2.0 * w))) / (1.0 - 2.0 * w);
    float y = t * (2.0 - 2.0 * w + 2.0 * t * w - t);

    return x;
}

/*
*/

/*
float2 adsrEncode(float start, float end, float attack, float decay, float release) {

}

float adsrbqerpDecode(float2 encoded, float sustain, float attackCurve, float decayCurve, float releaseCurve) {

}

float adsrbqerp(float start, float end, float attack, float decay, float sustain, float release, float attackCurve = 0.0, float decayCurve = 0.0, float releaseCurve = 0.0) {

}
*/

float adsrFixedEncoder(float start, float attackDuration, float decayDuration, float sustainDuration, float releaseDuration) {
    if (start <= 0.0) {
        return 0.0;
    } else if (start < attackDuration) {
        return start / attackDuration;
    } else if (start < attackDuration + decayDuration) {
        return (start - attackDuration) / decayDuration + 1.0;
    } else if (start < attackDuration + decayDuration + sustainDuration) {
        return 2.0;
    } else if (start < attackDuration + decayDuration + sustainDuration + releaseDuration) {
        return (start - attackDuration - decayDuration - sustainDuration) / releaseDuration + 2.0;
    }

    return 0.0;
}

float adsrEncoder(float start, float end, float attackDuration, float decayDuration, float releaseDuration) {
    float value;
    if (start <= 0.0) {
        value = 0.0;
    } else if (start < attackDuration) {
        value = start / attackDuration;
    } else if (start < attackDuration + decayDuration) {
        value = (start - attackDuration) / decayDuration + 1.0;
    } else {
        value = 2.0;
    }

    if (end <= 0.0) {
        return value;
    }

    return end / releaseDuration + 2.0;
}

#define adsrInterpolate(t_, sustain_, attackInterpolate_, decayInterpolate_, releaseInterpolate_) \
    ((t_) < 0.0 || (t_) > 3.0) \
    ? (0.0) \
    : (((t_) < 1.0) \
        ? (attackInterpolate_(t_)) \
        : (((t_) < 2.0) \
            ? ((1.0 - (sustain_)) * decayInterpolate_(2.0 - (t_)) + (sustain_)) \
            : (((sustain_) * releaseInterpolate_(3.0 - (t_)))) \
        ) \
    )

float sampleNoise(uint64_t id, float frequency) {
    return sampleFMSine(id, frequency, 1000000.0, frequency * pi);
}

float decayCurve(float t, float a, float b, float c) {
    return a - (a - b) * (exp(c * t) - 1) / (exp(c) - 1);
}

float decayCurveIntegrated(float t, float a, float b, float c) {
    return a * t + (a - b) / (exp(c) - 1) * (t - exp(c * t) / c);
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
    BezierQuadInterpolator bqi;

    float start = fmod(t, 2.0) - 0.5;
    float end = fmod(t, 2.0) - 1.5;

    float headCarrierFrequency = 150.0;
    float headModulationFrequency = 180.0;
    float headGainEnvelope = adsrInterpolate(adsrEncoder(start, end, 0.000, 0.090, 0.005), 0.0, bqi.interpolator(1.0).interpolate, bqi.interpolator(-0.2).interpolate, bqi.interpolator(-1.0).interpolate);
    float headModulationEnvelope = adsrInterpolate(adsrEncoder(start, end, 0.005, 0.050, 0.005), 0.0, bqi.interpolator(1.0).interpolate, bqi.interpolator(1.0).interpolate, bqi.interpolator(1.0).interpolate);
    float headSample = sampleFMPitchEnvelopeSine(id, start, headCarrierFrequency * 7.0, headCarrierFrequency, 0.010, 2.0 * headModulationEnvelope, headModulationFrequency);

    float snareGainEnvelope = adsrInterpolate(adsrEncoder(start, end, 0.005, 0.055, 0.005), 0.0, bqi.interpolator(1.0).interpolate, bqi.interpolator(0.5).interpolate, bqi.interpolator(1.0).interpolate);
    float snareFrequency = 120.0;
    float snareSample = sampleNoise(id, snareFrequency);

    float v = sampleFMSine(id, 220.0, 1.0, 55.0);
    //float v = headGainEnvelope * headSample + 0.6 * snareGainEnvelope * snareSample;
    v = clamp(v, -1.0, 1.0);

    //rawWriteToTile(tiles[0], float2(float(localID) / float(32768.0 * 2.0), lmap(encoded.x, 0.0, -3.0, 0.0, 1.0)), color);

    //audioBuffer[localID] = v;
    writeSampleToPushConstantBuffer(pushConstants.buffer, localID, v);
}
