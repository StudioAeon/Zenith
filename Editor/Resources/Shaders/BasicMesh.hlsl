#pragma pack_matrix(column_major)
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
};

struct FragmentOutput
{
	float4 Color : SV_Target0;
};

struct PushConstants
{
	float4x4 u_Transform;
	float4x4 u_ViewProjection;
	float4x4 u_NormalMatrix;
};

[[vk::push_constant]] PushConstants pc;

#pragma stage : vert
VertexOutput main(VertexInput input)
{
	VertexOutput output;

	float4 worldPos = mul(pc.u_Transform, float4(input.Position, 1.0));
	output.Position = mul(pc.u_ViewProjection, worldPos);

	output.WorldPos = worldPos.xyz;
	output.Normal = normalize(mul((float3x3)pc.u_NormalMatrix, input.Normal));

	return output;
}

#pragma stage : frag
FragmentOutput main(VertexOutput input)
{
	FragmentOutput output;

	float3 lightDir = normalize(float3(-0.5, -1.0, -0.3));
	float3 viewPos = float3(0.0, 1.0, 3.0);

	float3 n = normalize(input.Normal);
	float3 viewDir = normalize(viewPos - input.WorldPos);
	float3 lightColor = float3(1.0, 1.0, 1.0);
	float3 surfaceColor = float3(0.2, 0.35, 0.85);

	float ambientStrength = 0.1;
	float3 ambient = ambientStrength * lightColor;

	float diff = max(dot(n, -lightDir), 0.0);
	float3 diffuse = diff * lightColor;

	float3 reflectDir = reflect(lightDir, n);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
	float3 specular = 0.4 * spec * lightColor;

	float3 result = (ambient + diffuse + specular) * surfaceColor;
	output.Color = float4(result, 1.0);

	return output;
}
