[[vk::binding(0, 0)]]
Texture2D<float4> inputImage;

[[vk::binding(1, 0)]]
[format("rgba8")]
RWTexture2D<float4> outputImage;

static const int bayer4x4[16] = {
     0, 8, 2, 10,
    12, 4, 14, 6,
     3, 11, 1, 9,
    15, 7, 13, 5
};

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
    uint2 index = threadId.xy;

    // Check if valid index
    uint width, height;
    outputImage.GetDimensions(width, height);
    if (index.x >= width || index.y >= height) {
        return;
    }

    uint pixelScale = 2;
    uint2 virtualIndex = (index / pixelScale) * pixelScale;
    float3 inColor = inputImage.Load(int3(virtualIndex.x, virtualIndex.y, 0)).rgb;

    // Apply bayer equation from https://en.wikipedia.org/wiki/Ordered_dithering
    uint2 ditherCoord = index / pixelScale;
    float colorBitsPerChannel = 31.0;
    int bayerSize = 4;

    int bayerIndex = ((ditherCoord.x % bayerSize) + (bayerSize * (ditherCoord.y % bayerSize)));

    // So first we do Bayer value -> Normalize 0-1
    // Now we apply this positive value and floor, meaning that as a float approaches a breakpoint, more and more of the pixels become the new color level
    float bayerScalar = (bayer4x4[bayerIndex] * (1.0 / (bayerSize * bayerSize)));

    float3 ditheredColor = inColor + (bayerScalar / colorBitsPerChannel);
    //float3 ditheredColor = inColor;

    outputImage[index] = float4(floor(ditheredColor * colorBitsPerChannel) / colorBitsPerChannel, 1.0);

    //outputImage[index] = inputImage.Load(int3(index.x, index.y, 0));
}