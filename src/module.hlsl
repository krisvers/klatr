#define INTERNAL_AUDIO_BUFFER_FRAME_COUNT 64

#define groupDimensions uint3(1, 1, 1)
#define dispatchDimensions uint3(INTERNAL_AUDIO_BUFFER_FRAME_COUNT, 1, 1)

struct PushConstantAudioBufferDescriptor {
    uint64_t address;
    float2 bounds; /* note: if bounds.x == bounds.y this is treated as a constant */
    uint count;
};

struct UniformAudioBufferDescriptor {
    float2 bounds; /* note: if bounds.x == bounds.y this is treated as a constant */
    uint count;
};

[[vk::push_constant]]
PushConstantAudioBufferDescriptor pushConstants;

[[vk::binding(0, 0)]]
ConstantBuffer<UniformAudioBufferDescriptor> uniforms;

[[vk::binding(1, 0)]]
RWStructuredBuffer<float> audioBuffer;

[numthreads(1, 1, 1)]
void module(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint3 threadCoordinateID : SV_DispatchThreadID) { 
    uint id = groupThreadID.x + groupDimensions.x * (groupThreadID.y + groupDimensions.y * (groupThreadID.z + groupDimensions.z * (groupID.x + dispatchDimensions.x * (groupID.y + dispatchDimensions.y * groupID.z))));
    //vk::RawBufferStore<float>(pushConstants.address + id * 4, sin(float(id)));
    audioBuffer[id] = sin(float(id));
}