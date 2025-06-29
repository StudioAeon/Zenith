struct VertexInput
{
	float3 Position : POSITION;
};

struct VertexOutput
{
	float4 Position : SV_Position;
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

	return output;
}

#pragma stage : frag
FragmentOutput main(VertexOutput input)
{
	FragmentOutput output;

	output.Color = float4(0.6, 0.15, 0.15, 1.0);

	return output;
}