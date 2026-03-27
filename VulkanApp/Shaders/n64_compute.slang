[[vk::binding(0, 0)]]
Texture2D<float4> inputImage;

[[vk::binding(1, 0)]]
[format("rgba8")]
RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
    uint2 index = threadId.xy;

    uint width, height;
    outputImage.GetDimensions(width, height);
    if (index.x >= width || index.y >= height) {
        return;
    }

    float colorBitsPerChannel = 32.0;
    float3 inColor = inputImage.Load(int3(index.x, index.y, 0)).rgb;

    outputImage[index] = float4(round(inColor * colorBitsPerChannel) / colorBitsPerChannel, 1.0); 
}