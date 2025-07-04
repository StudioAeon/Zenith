// Basic Mesh shader

#version 450 core
#pragma stage : vert

layout(push_constant) uniform PushConstants {
	mat4 u_Transform;
	mat4 u_ViewProjection;
	mat4 u_NormalMatrix;
} pc;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_WorldPos;
layout(location = 1) out vec3 v_Normal;

void main()
{
	vec4 worldPos = pc.u_Transform * vec4(a_Position, 1.0);
	gl_Position = pc.u_ViewProjection * worldPos;

	v_WorldPos = worldPos.xyz;
	v_Normal = normalize(mat3(pc.u_NormalMatrix) * a_Normal);
}

#version 450 core
#pragma stage : frag

layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;

layout(location = 0) out vec4 color;

void main()
{
	vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.3));
	vec3 viewPos = vec3(0.0, 1.0, 3.0);

	vec3 n = normalize(v_Normal);
	vec3 viewDir = normalize(viewPos - v_WorldPos);
	vec3 lightColor = vec3(1.0);
	vec3 surfaceColor = vec3(0.2, 0.35, 0.85);

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;

	float diff = max(dot(n, -lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	vec3 reflectDir = reflect(lightDir, n);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
	vec3 specular = 0.4 * spec * lightColor;

	vec3 result = (ambient + diffuse + specular) * surfaceColor;
	color = vec4(result, 1.0);
}
