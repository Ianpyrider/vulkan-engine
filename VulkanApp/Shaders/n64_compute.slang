[vk::image_format("r32f")]
Texture2D<float4> inputImage;

[vk::image_format("r32f")]
RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
    uint index = threadId.x;

    outputImage[index] = float4(1.0, 0.0, 0.0, 1.0); 
}