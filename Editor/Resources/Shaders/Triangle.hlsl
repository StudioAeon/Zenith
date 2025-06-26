struct VertexInput
{
    float3 Position : POSITION;
    float3 Color    : COLOR;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float3 Color    : TEXCOORD0;
};

struct FragmentOutput
{
    float4 Color : SV_Target0;
};

#pragma stage : vert
VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.Position = float4(input.Position, 1.0);
    output.Color = input.Color;
    return output;
}

#pragma stage : frag
FragmentOutput main(VertexOutput input)
{
    FragmentOutput output;
    output.Color = float4(input.Color, 1.0);
    return output;
}