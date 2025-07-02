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
	float Transparency;
	float Emission;
	bool UseNormalMap;
	float2 _Padding;
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

[[vk::binding(4, 0)]] SamplerState u_Sampler : register(s0);

float3 CalculateTransparentLighting(float3 albedo, float3 normal, float3 viewDir, DirectionalLight light)
{
	float3 lightDir = normalize(-light.Direction);
	float NdotL = saturate(dot(normal, lightDir));
	float NdotV = saturate(dot(normal, viewDir));

	float3 diffuse = albedo * NdotL;

	float fresnel = pow(1.0 - NdotV, 3.0);
	float3 fresnelColor = lerp(albedo, float3(1.0, 1.0, 1.0), fresnel * 0.3);

	float rim = 1.0 - NdotV;
	float3 rimColor = albedo * pow(rim, 2.0) * 0.4;
	
	return (diffuse + rimColor) * light.Color * light.Intensity + fresnelColor * 0.1;
}

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

	float4 albedoSample = u_AlbedoTexture.Sample(u_Sampler, input.TexCoord);
	float3 albedo = albedoSample.rgb * u_MaterialUniforms.AlbedoColor;
	float alpha = albedoSample.a * u_MaterialUniforms.Transparency;

	if (alpha < 0.005)
		discard;

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
	mainLight.Intensity = 2.5;
	
	float3 viewDir = normalize(u_Camera.CameraPosition - input.WorldPos);

	float3 color = CalculateTransparentLighting(albedo, normal, viewDir, mainLight);

	float3 ambient = albedo * 0.08;
	color += ambient;

	color += albedo * u_MaterialUniforms.Emission;

	color = color / (color + 0.8);

	color = pow(color, 1.0 / 2.2);
	
	output.Color = float4(color, alpha);
	return output;
}