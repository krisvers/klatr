#define INTERNAL_AUDIO_BUFFER_FRAME_COUNT 65536

#define groupDimensions uint3(1, 1, 1)
#define dispatchDimensions uint3(INTERNAL_AUDIO_BUFFER_FRAME_COUNT, 1, 1)

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
    uint sampleRate;
    UniformAudioBufferDescriptor buffer;
};

[[vk::push_constant]]
PushConstants pushConstants;

[[vk::binding(0, 0)]]
ConstantBuffer<Uniforms> uniforms;

[[vk::binding(1, 0)]]
RWStructuredBuffer<float> audioBuffer;

[numthreads(1, 1, 1)]
void module(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint3 threadCoordinateID : SV_DispatchThreadID) {
    const float pi = 3.14159265358979323846f;

    uint localID = groupThreadID.x + groupDimensions.x * (groupThreadID.y + groupDimensions.y * (groupThreadID.z + groupDimensions.z * (groupID.x + dispatchDimensions.x * (groupID.y + dispatchDimensions.y * groupID.z))));
    uint64_t id = localID / 2 + uniforms.globalID; /* scuffed temporary patch of mixing samples and frames */

    float t = float(id) / float(uniforms.sampleRate);

    float v = 2.0 * fmod(t * 440.0, 1.0) - 1.0;
    //float v = sin(2 * pi * t * 110.0);
    //vk::RawBufferStore<float>(pushConstants.address + id * 4, v);

    audioBuffer[localID] = v;
}
