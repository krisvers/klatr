struct PushConstantAudioBufferDescriptor {
    uint64_t address;
    float2 bounds; /* note: if bounds.x == bounds.y this is treated as a constant */
    uint count;
};

struct PushConstants {
    PushConstantAudioBufferDescriptor buffer;
};

struct UniformAudioBufferDescriptor {
    float2 bounds; /* note: if bounds.x == bounds.y this is treated as a constant */
    uint count;
};

struct Uniforms {
    uint64_t globalID;
    float globalTime;
    uint dispatchWidth;
    uint sampleRate;
    float inverseSampleRate;
    UniformAudioBufferDescriptor buffer;
};

[[vk::push_constant]]
PushConstants pushConstants;

[[vk::binding(0, 0)]]
ConstantBuffer<Uniforms> uniforms;

[[vk::binding(1, 0)]]
RWStructuredBuffer<float> audioBuffer;

#define pi float(3.14159265358979323846f)

float angleFromSamplePeriod(uint64_t id, uint sampleRate, float period) {
    float scale = 1000;
    
    uint64_t scaledPeriodInSamples = uint64_t(period * sampleRate * scale);
    float scaledAngle = float((id * scale) % scaledPeriodInSamples);
    return scaledAngle / float(scale) / float(sampleRate) / period * 2.0 * pi;
}

float sampleSine(uint64_t id, uint sampleRate, float frequency, float phase = 0.0, float min = -1.0, float max = 1.0) {
    float range = max - min;
    return (sin(angleFromSamplePeriod(id, sampleRate, 1.0 / frequency) + phase) / 2.0 + 0.5) * range + min;
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
    float v = sampleSine(id, uniforms.sampleRate, sampleSine(id, uniforms.sampleRate, 1, 0.0, 110.0, 220.0));
    //float v = sin(2.0 * pi * t * 220.0);
    //vk::RawBufferStore<float>(pushConstants.address + id * 4, v);

    audioBuffer[localID] = v;
}
