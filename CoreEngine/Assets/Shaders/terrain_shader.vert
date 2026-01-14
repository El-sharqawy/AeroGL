#version 460 core

layout (location = 0) in vec3 m_v3Position;
layout (location = 1) in vec3 m_v3Normals;
layout (location = 2) in vec2 m_v2TexCoord;
layout (location = 3) in vec4 m_v4Color;

layout (std140, binding = 0) uniform CameraData
{
    mat4 View;
    mat4 Projection;
    mat4 ViewProjection;
} camera;

uniform mat4 u_matViewProjection;
uniform mat4 u_matModel;
out vec3 v3Position;
out vec3 v3Normals;
out vec2 v2TexCoord;
out vec4 v4Color;

void main()
{
	gl_Position =  u_matViewProjection * u_matModel * vec4(m_v3Position, 1.0);

	v3Position = m_v3Position;
	v3Normals = m_v3Normals;
	v2TexCoord = m_v2TexCoord;
	v4Color = m_v4Color;
}
