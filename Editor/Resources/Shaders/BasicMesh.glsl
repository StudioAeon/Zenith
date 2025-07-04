#version 450 core
#pragma stage : vert

layout(push_constant) uniform PushConstants {
	mat4 u_Model;
	mat4 u_ViewProjection;
	mat4 u_NormalMatrix;
	vec4 u_CameraPosition;
} pc;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_WorldPosition;
layout(location = 1) out vec3 v_Normal;

void main()
{
	vec4 worldPosition = pc.u_Model * vec4(a_Position, 1.0);
	v_WorldPosition = worldPosition.xyz;

	gl_Position = pc.u_ViewProjection * worldPosition;
	gl_Position.y = -gl_Position.y;

	v_Normal = normalize((pc.u_NormalMatrix * vec4(a_Normal, 0.0)).xyz);
}

#version 450 core
#pragma stage : frag

layout(push_constant) uniform PushConstants {
	mat4 u_Model;
	mat4 u_ViewProjection;
	mat4 u_NormalMatrix;
	vec4 u_CameraPosition;
} pc;

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_Normal;
layout(location = 0) out vec4 color;

void main()
{
	vec3 albedo = vec3(0.2, 0.3, 0.8);
	float shininess = 18.0;

	vec3 lightPosition = vec3(10.0, 10.0, 10.0);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	vec3 ambientColor = vec3(0.2, 0.2, 0.2);

	vec3 normal = normalize(v_Normal);
	vec3 lightDir = normalize(lightPosition - v_WorldPosition);
	vec3 viewDir = normalize(pc.u_CameraPosition.xyz - v_WorldPosition);

	vec3 ambient = ambientColor * albedo;

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * lightColor * albedo;

	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 specular = spec * lightColor;

	vec3 finalColor = ambient + diffuse + specular;

	color = vec4(finalColor, 1.0);
}