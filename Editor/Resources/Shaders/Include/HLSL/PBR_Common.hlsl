#ifndef PBR_COMMON_HLSL
#define PBR_COMMON_HLSL

struct MaterialProperties
{
    float3 Albedo;
    float3 Normal;
    float Metalness;
    float Roughness;
    float AO; // Ambient Occlusion
    float3 Emission;
    float Alpha;
};

struct DirectionalLight
{
    float3 Direction;
    float3 Color;
    float Intensity;
};

static const float PI = 3.14159265359;
static const float MIN_ROUGHNESS = 0.04;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 CalculatePBRLighting(MaterialProperties material, float3 N, float3 V, DirectionalLight light)
{
    float3 L = normalize(-light.Direction);
    float3 H = normalize(V + L);
    
    float3 radiance = light.Color * light.Intensity;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, material.Roughness);
    float G = GeometrySmith(N, V, L, material.Roughness);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), material.Albedo, material.Metalness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - material.Metalness;
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    float3 specular = numerator / max(denominator, 0.001);
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * material.Albedo / PI + specular) * radiance * NdotL;
}

float3 CalculateSimplifiedPBR(float3 albedo, float metalness, float roughness, float3 N, float3 V, float3 L, float3 lightColor)
{
    float3 H = normalize(V + L);
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    float3 diffuse = albedo * (1.0 - metalness) * NdotL;

    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    
    // Simplified D term
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (PI * denom * denom);
    
    // Simplified G term
    float k = alpha / 2.0;
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float G = G1L * G1V;
    
    // Simplified F term
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metalness);
    float3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
    
    float3 specular = (D * G * F) / (4.0 * NdotL * NdotV + 0.001);
    
    return (diffuse + specular) * lightColor * NdotL;
}

float3 SampleNormalMap(Texture2D normalMap, SamplerState sampler, float2 uv, float3x3 TBN)
{
    float3 normalSample = normalMap.Sample(sampler, uv).rgb * 2.0 - 1.0;
    return normalize(mul(normalSample, TBN));
}

// Create TBN matrix
float3x3 CreateTBN(float3 normal, float3 tangent, float3 binormal)
{
    return float3x3(
        normalize(tangent),
        normalize(binormal),
        normalize(normal)
    );
}

#endif