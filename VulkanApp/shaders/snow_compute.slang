struct Particle {
    float4 position; // Use float4 for guaranteed 16-byte alignment
    float4 velocity; 
};

struct UniformBuffer {
    float deltaTime;
    float totalTime;
};

ConstantBuffer<UniformBuffer> ubo;

struct ParticleSSBO {
    Particle particles;
};

[[vk::binding(1, 0)]] RWStructuredBuffer<Particle> particlesBuffer;

[shader("compute")]
[numthreads(256, 1, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
    uint index = threadId.x;
    if (index >= 4096) return;

    float3 pos = particlesBuffer[index].position.xyz;
    float3 baseVel = particlesBuffer[index].velocity.xyz;

    float windX = sin(pos.z * 1.5 + ubo.totalTime) * 0.4;
    float windY = cos(pos.z * 1.2 + ubo.totalTime * 0.8) * 0.4;

    pos.x += windX * ubo.deltaTime;
    pos.y += windY * ubo.deltaTime;
    pos.z += baseVel.z * ubo.deltaTime;

    float horizontalBounds = 15.0;
    float skyZ = 10.0;
    float groundZ = -2.0; 

    if (pos.z < groundZ) pos.z = skyZ;

    if (pos.x < -horizontalBounds) pos.x += horizontalBounds * 2.0;
    if (pos.x > horizontalBounds) pos.x -= horizontalBounds * 2.0;
    if (pos.y < -horizontalBounds) pos.y += horizontalBounds * 2.0;
    if (pos.y > horizontalBounds) pos.y -= horizontalBounds * 2.0;

    particlesBuffer[index].position = float4(pos, 1.0);
    particlesBuffer[index].velocity = float4(baseVel, 0.0);
}