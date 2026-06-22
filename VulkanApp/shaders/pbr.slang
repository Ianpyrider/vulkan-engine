// Combined vertex and fragment shader for PBR rendering

// Input from vertex buffer - Data sent per vertex from CPU
struct VSInput {
    float3 Position : POSITION; // 3D position in model space
    float3 Color : COLOR;
    float3 Normal : NORMAL;            // Surface normal for lighting calculations
    float2 UV : TEXCOORD0;          // Texture coordinates for material sampling
    // float4 Tangent : TANGENT;       // Tangent vector for normal mapping (w component = handedness)
    // float2 Bitangent : BITANGENT;
};

// Output from vertex shader / Input to fragment shader - Interpolated data
struct VSOutput {
    float4 Position : SV_POSITION; // Required clip space position for rasterization
    float3 WorldPos : POSITION;    // World space position for lighting calculations
    float3 Normal : NORMAL;        // World space normal (interpolated)
    float3 Color : COLOR;
    float2 UV : TEXCOORD0; // Texture coordinates (interpolated)
    // float4 Tangent : TANGENT;      // World space tangent (interpolated)
};

// Uniform buffer - Global data shared across all vertices/fragments
struct UniformBufferObject {
    float4x4 model;                     // Model-to-world transformation matrix
    float4x4 view;                      // World-to-camera transformation matrix
    float4x4 proj;                      // Camera-to-clip space projection matrix
    float4 lightPositions[4];           // Light positions in world space
    float4 lightColors[4];              // Light intensities and colors
    float4 camPos;                      // Camera position for view-dependent effects
    float exposure;                     // HDR exposure control
    float gamma;                        // Gamma correction value (typically 2.2)
    float prefilteredCubeMipLevels;     // IBL prefiltered environment map mip levels
    float scaleIBLAmbient;              // IBL ambient contribution scale
};

// Push constants - Fast, small data updated frequently per material/object
struct PushConstants {
    float4 baseColorFactor;             // Base color tint/multiplier
    float metallicFactor;               // Metallic property multiplier
    float roughnessFactor;              // Surface roughness multiplier
    int baseColorTextureSet;            // Texture binding index for base color (-1 = none)
    int physicalDescriptorTextureSet;   // Texture binding for metallic/roughness
    int normalTextureSet;               // Texture binding for normal maps
    int occlusionTextureSet;            // Texture binding for ambient occlusion
    int emissiveTextureSet;             // Texture binding for emissive maps
    float alphaMask;                    // Alpha masking enable flag
    float alphaMaskCutoff;              // Alpha cutoff threshold
};

// Mathematical constants
static const float PI = 3.14159265359;

// Resource bindings - Connect CPU resources to GPU shader registers
[[vk::binding(0, 0)]] ConstantBuffer<UniformBufferObject> ubo;
[[vk::binding(0, 1)]] SamplerCube irradianceMap;
[[vk::binding(1, 1)]] SamplerCube radianceMap;
[[vk::binding(2, 1)]] Sampler2D brdfLUT;

[[vk::push_constant]] PushConstants material;

// Normal Distribution Function (D) - GGX/Trowbridge-Reitz Distribution
// Describes the statistical distribution of microfacet orientations
float DistributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness; // Remapping for more perceptual linearity
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float nom = a2; // Numerator: concentration factor
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom; // Normalization factor

    return nom / denom; // Normalized distribution
}

// Geometry Function (G) - Smith's method with Schlick-GGX approximation
// Models self-shadowing and masking between microfacets
float GeometrySmith(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Direct lighting remapping

    // Geometry obstruction from view direction (masking)
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    // Geometry obstruction from light direction (shadowing)
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);

    return ggx1 * ggx2; // Combined masking-shadowing
}

// Fresnel Reflectance (F) - Schlick's approximation
// Models how reflectance changes with viewing angle
float3 FresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 worldPos = mul(ubo.model, float4(input.Position, 1.0));
    output.WorldPos = worldPos.xyz;
    output.Position = mul(ubo.proj, mul(ubo.view, worldPos));

    output.Normal = mul((float3x3)ubo.model, input.Normal);
    output.Color = input.Color;
    output.UV = input.UV;
    
    return output;
}

/*
[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float3 lookupDirection = float3(input.Normal.x, input.Normal.z, input.Normal.y);
    return environmentMap.Sample(lookupDirection);

    //return float4(input.Color, 1.0);
}
*/

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float3 N = normalize(input.Normal);
    //float3 R = N;
    //float3 V = R;

    float3 V = normalize(ubo.camPos.xyz - input.WorldPos);
    float3 R = reflect(-V, N);

    // float3 albedo = input.Color * material.baseColorFactor.rgb;
    float3 albedo = input.Color;

    // === PBR MATERIAL SETUP ===
    // Calculate F0 (reflectance at normal incidence)
    // Non-metals: low reflectance (~0.04), Metals: colored reflectance from base color
    float3 F0 = float3(0.04, 0.04, 0.04);   // Dielectric default
    //float3 F0 = material.baseColorFactor.xyz;

    F0 = lerp(F0, albedo, material.metallicFactor); // Lerp to metallic behavior

    // Initialize outgoing radiance accumulator
    float3 Lo = float3(0.0, 0.0, 0.0);

    float NdotV = max(dot(N, V), 0.01); // View angle

    // === DIRECT LIGHTING LOOP ===
    // Calculate contribution from each light source
    for (int i = 0; i < 4; i++) {

        float3 lightPos = ubo.lightPositions[i].xyz;
        float3 lightColor = ubo.lightColors[i].rgb;

        // Calculate light direction and attenuation
        float3 L = normalize(lightPos - input.WorldPos);    // Light direction
        float distance = length(lightPos - input.WorldPos); // Distance for falloff

        float attenuation = 1.0 / (distance * distance); // Inverse square falloff

        float3 radiance = lightColor * attenuation;         // Attenuated light color

        // Calculate half vector (between view and light directions)
        float3 H = normalize(V + L);

        // === BRDF EVALUATION ===
        // Calculate all necessary dot products for BRDF terms
        float NdotL = max(dot(N, L), 0.01); // Lambertian falloff
        float NdotH = max(dot(N, H), 0.0); // Half vector for specular
        float HdotV = max(dot(H, V), 0.0); // For Fresnel calculation

        // Evaluate Cook-Torrance BRDF components
        float D = DistributionGGX(NdotH, material.roughnessFactor);      // Normal distribution
        float G = GeometrySmith(NdotV, NdotL, material.roughnessFactor); // Geometry function
        float3 F = FresnelSchlick(HdotV, F0);             // Fresnel reflectance

        // Calculate specular BRDF
        float3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001; // Prevent division by zero
        float3 specular = numerator / denominator;

        // === ENERGY CONSERVATION ===
        // Fresnel term represents specular reflection ratio
        float3 kS = F;                          // Specular contribution
        float3 kD = float3(1.0, 1.0, 1.0) - kS; // Diffuse contribution (energy conservation)
        kD *= 1.0 - material.metallicFactor;   // Metals have no diffuse reflection

        // === RADIANCE ACCUMULATION ===
        // Combine diffuse (Lambertian) and specular (Cook-Torrance) terms
        // Multiply by incident radiance and cosine foreshortening
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // IBL

    float ao = 1.0; // hardcoded for now
    float emissive = 0.0;
    
    float3 sampleRadiance = radianceMap.SampleLevel(float3(R.x, R.z, R.y), material.roughnessFactor * ubo.prefilteredCubeMipLevels).xyz * ubo.exposure;

    float2 sampleBRDF = brdfLUT.Sample(float2(NdotV, 1.0 - material.roughnessFactor)).xy;

    float3 kS = F0 * sampleBRDF.x + sampleBRDF.y;

    float3 specularIBL = sampleRadiance * kS;

    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - material.metallicFactor;

    float3 sampleIrradiance = irradianceMap.Sample(float3(input.Normal.x, input.Normal.z, input.Normal.y)).xyz;
    float3 diffuseIBL = sampleIrradiance * ubo.exposure * albedo;

    float3 ambient = (kD * diffuseIBL + specularIBL) * ao;
    
    float3 color = ambient + Lo + emissive;

    // === HDR TONE MAPPING AND GAMMA CORRECTION ===
    // Apply Reinhard tone mapping to compress HDR values to [0,1] range
    color = color / (color + float3(1.0, 1.0, 1.0));

    // Apply gamma correction for sRGB display (inverse gamma)
    float gamma = ubo.gamma;
    color = pow(color, float3(1.0 / gamma, 1.0 / gamma, 1.0 / gamma));

    // Output final color with original alpha
    return float4(color, material.baseColorFactor.a);
}
