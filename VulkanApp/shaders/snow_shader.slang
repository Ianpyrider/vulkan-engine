struct SnowVertexInput {
    [vk::location(0)] float3 inPosition;
};

struct CameraUBO {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<CameraUBO> camera;

struct SnowVertexOutput {
    float4 posOut : SV_Position;
    float pointSize : SV_PointSize;
};

[shader("vertex")]
SnowVertexOutput vertMain(SnowVertexInput input) {
    SnowVertexOutput output;
    output.posOut = mul(camera.proj, mul(camera.view, float4(input.inPosition, 1.0)));

    output.pointSize = 4.0;

    return output;
}

[shader("fragment")]
float4 fragMain() : SV_Target
{
    return float4(0.9, 0.9, 1.0, 1.0);
}
