[[vk::binding(0, 0)]]
Texture2D<float4> inputImage;

[[vk::binding(1, 0)]]
[format("rgba8")]
RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(1, 1, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
    uint2 index = threadId.xy;

    outputImage[index] = float4(1.0, 0.0, 0.0, 1.0); 
}