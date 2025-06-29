struct VertexInput
{
	float3 Position : POSITION;
	float3 Normal   : NORMAL;
	float3 Tangent  : TANGENT;
	float3 Binormal : BINORMAL;
	float2 TexCoord : TEXCOORD0;
};

struct VertexOutput
{
	float4 Position : SV_Position;
	float3 Normal   : TEXCOORD0;
	float3 WorldPos : TEXCOORD1;
};

struct FragmentOutput
{
	float4 Color : SV_Target0;
};

struct PushConstants
{
	float4x4 u_Transform;
};

[[vk::push_constant]] PushConstants pc;

#pragma stage : vert
VertexOutput main(VertexInput input)
{
	VertexOutput output;

	float4 worldPos = float4(input.Position, 1.0);
	output.Position = mul(worldPos, pc.u_Transform);

	output.Normal = input.Normal;
	output.WorldPos = input.Position;

	return output;
}

#pragma stage : frag
FragmentOutput main(VertexOutput input)
{
	FragmentOutput output;

	float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
	float3 normal = normalize(input.Normal);
	float ndotl = max(dot(normal, lightDir), 0.4);

	float3 baseColor = float3(0.8, 0.2, 0.2);
	float3 finalColor = baseColor * ndotl;

	output.Color = float4(finalColor, 1.0);
	return output;
}