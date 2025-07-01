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
	float2 TexCoord : TEXCOORD2;
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
	output.Position = mul(pc.u_Transform, worldPos);

	output.WorldPos = worldPos.xyz;
	output.Normal = input.Normal;
	output.TexCoord = input.TexCoord;

	return output;
}

#pragma stage : frag
FragmentOutput main(VertexOutput input, bool isFrontFace : SV_IsFrontFace)
{
	FragmentOutput output;

	//output.Color = isFrontFace ? float4(0, 1, 0, 1) : float4(1, 0, 0, 1);
	output.Color = float4(0.6, 0.15, 0.15, 1.0);

	//float3 color = (input.WorldPos + 10.0f) / 20.0f;
	//output.Color = float4(saturate(color), 1.0f);

	return output;
}
