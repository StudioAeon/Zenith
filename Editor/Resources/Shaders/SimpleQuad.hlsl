struct VertexInput {
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
};

struct VertexOutput {
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD0;
};

struct FragmentOutput {
	float4 Color : SV_Target0;
};

#pragma stage : vert
VertexOutput main(VertexInput input) {
	VertexOutput output;
	output.Position = float4(input.Position, 1.0);
	output.TexCoord = input.TexCoord;
	return output;
}

#pragma stage : frag
FragmentOutput main(VertexOutput input) {
	FragmentOutput output;

	float2 centeredUV = input.TexCoord - 0.5;
	float distance = length(centeredUV);
	float gradient = 1.0 - distance;

	float3 baseColor = float3(0.2, 0.3, 0.8);
	float3 finalColor = baseColor * (0.7 + 0.3 * gradient);

	output.Color = float4(finalColor, 1.0);
	return output;
}