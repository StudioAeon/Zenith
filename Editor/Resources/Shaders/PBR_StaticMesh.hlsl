#include "Include/HLSL/PBR_Common.hlsl"

struct VertexInput
{
	float3 Position  : POSITION;
	float3 Normal    : NORMAL;
	float3 Tangent   : TANGENT;
	float3 Binormal  : BINORMAL;
	float2 TexCoord  : TEXCOORD0;
};

struct VertexOutput
{
	float4 Position : SV_Position;
	float3 WorldPos : TEXCOORD0;
	float3 Normal   : TEXCOORD1;
	float3 Tangent  : TEXCOORD2;
	float3 Binormal : TEXCOORD3;
	float2 TexCoord : TEXCOORD4;
};

struct FragmentOutput
{
	float4 Color : SV_Target0;
};

struct PushConstants
{
	float4x4 u_Transform;
};

struct MaterialUniforms
{
	float3 AlbedoColor;
	float Metalness;
	float Roughness;
	float Emission;
	bool UseNormalMap;
	float _Padding;
};

struct CameraUniforms
{
	float4x4 ViewProjection;
	float3 CameraPosition;
	float _Padding;
};

[[vk::push_constant]] PushConstants pc;

[[vk::binding(0, 0)]] cbuffer MaterialUniformBuffer : register(b0)
{
	MaterialUniforms u_MaterialUniforms;
}

[[vk::binding(1, 0)]] cbuffer CameraUniformBuffer : register(b1)
{
	CameraUniforms u_Camera;
}

[[vk::binding(2, 0)]] Texture2D u_AlbedoTexture : register(t0);
[[vk::binding(3, 0)]] Texture2D u_NormalTexture : register(t1);
[[vk::binding(4, 0)]] Texture2D u_MetalnessTexture : register(t2);
[[vk::binding(5, 0)]] Texture2D u_RoughnessTexture : register(t3);

[[vk::binding(6, 0)]] SamplerState u_Sampler : register(s0);

#pragma stage : vert
VertexOutput main(VertexInput input)
{
	VertexOutput output;

	float4 worldPos = float4(input.Position, 1.0);
	output.Position = mul(pc.u_Transform, worldPos);

	output.WorldPos = worldPos.xyz;
	output.Normal = normalize(input.Normal);
	output.Tangent = normalize(input.Tangent);
	output.Binormal = normalize(input.Binormal);
	output.TexCoord = input.TexCoord;

	return output;
}

#pragma stage : frag
FragmentOutput main(VertexOutput input, bool isFrontFace : SV_IsFrontFace)
{
	FragmentOutput output;

	float3 albedo = u_AlbedoTexture.Sample(u_Sampler, input.TexCoord).rgb * u_MaterialUniforms.AlbedoColor;
	float metalness = u_MetalnessTexture.Sample(u_Sampler, input.TexCoord).b * u_MaterialUniforms.Metalness;
	float roughness = u_RoughnessTexture.Sample(u_Sampler, input.TexCoord).g * u_MaterialUniforms.Roughness;

	roughness = max(roughness, MIN_ROUGHNESS);

	float3 normal = normalize(input.Normal);
	if (u_MaterialUniforms.UseNormalMap)
	{
		float3x3 TBN = CreateTBN(input.Normal, input.Tangent, input.Binormal);
		normal = SampleNormalMap(u_NormalTexture, u_Sampler, input.TexCoord, TBN);
	}

	if (!isFrontFace)
		normal = -normal;

	DirectionalLight mainLight;
	mainLight.Direction = float3(0.5, -1.0, 0.3);
	mainLight.Color = float3(1.0, 1.0, 1.0);
	mainLight.Intensity = 3.0;
	
	float3 viewDir = normalize(u_Camera.CameraPosition - input.WorldPos);

	float3 lightDir = normalize(-mainLight.Direction);
	float3 color = CalculateSimplifiedPBR(albedo, metalness, roughness, normal, viewDir, lightDir, mainLight.Color * mainLight.Intensity);

	float3 ambient = albedo * 0.03;
	color += ambient;

	color += albedo * u_MaterialUniforms.Emission;

	color = color / (color + 1.0);

	color = pow(color, 1.0 / 2.2);

	output.Color = float4(color, 1.0);
	return output;
}