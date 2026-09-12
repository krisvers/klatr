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

float angleFromSamplePeriod(uint64_t id, uint sampleRate, float period) {
    uint periodInSamples = uint(period * sampleRate);
    return float(id % periodInSamples) / float(sampleRate) / period;
}

[numthreads(32, 32, 1)]
void module(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint3 threadCoordinateID : SV_DispatchThreadID) {
    const float pi = 3.14159265358979323846f;

    const uint3 groupDimensions = uint3(32, 32, 1);
    const uint3 dispatchDimensions = uint3(uniforms.dispatchWidth, 1, 1);

    uint localID = groupThreadID.x + groupDimensions.x * (groupThreadID.y + groupDimensions.y * (groupThreadID.z + groupDimensions.z * (groupID.x + dispatchDimensions.x * (groupID.y + dispatchDimensions.y * groupID.z))));
    uint64_t id = localID + uniforms.globalID;

    //float v = sign(fmod(angleFromSamplePeriod(id, uniforms.sampleRate, 1.0 / 220.0), 1.0) - 0.5);
    //float v = 2.0 * fmod(t * 220.0, 1.0) - 1.0;
    float v = sin(angleFromSamplePeriod(id, uniforms.sampleRate, pi / (220.0)));
    //vk::RawBufferStore<float>(pushConstants.address + id * 4, v);

    audioBuffer[localID] = v;
}
