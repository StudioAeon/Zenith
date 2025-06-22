struct VSInput {
	float2 a_Position : POSITION;
	float3 a_Color    : COLOR;
};

struct VSOutput {
	float4 Position : SV_POSITION;
	float3 Color    : COLOR0;
};

#ifdef VERTEX_SHADER
VSOutput main(VSInput input) {
	VSOutput output;
	output.Position = float4(input.a_Position, 0.0, 1.0);
	output.Color = input.a_Color;
	return output;
}
#endif

#ifdef FRAGMENT_SHADER
float4 main(VSOutput input) : SV_TARGET {
	return float4(input.Color, 1.0);
}
#endif