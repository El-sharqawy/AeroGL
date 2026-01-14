#version 460 core

layout (location = 0) in vec3 m_v3Position;
layout (location = 1) in vec2 m_v2TexCoord;
layout (location = 2) in vec4 m_v4Color;

#define MODERN_OPENGL_PATH

#ifdef MODERN_OPENGL_PATH
// BINDING 1: The "World" (All Object Positions)
layout(std430, binding = 0) readonly buffer MatrixBuffer
{
    mat4 modelMatrices[]; // Leave empty [] to match the SSBO size automatically
    // Note: If on 4.3 but not 4.6, you might need an extension for gl_DrawID
    // or pass it as a vertex attribute.
};
#else
uniform mat4 u_matModel = mat4(1.0f);
#endif

// Camera UBO (works on OpenGL 3.1+)
layout (std140, binding = 0) uniform CameraData
{
    mat4 View;
    mat4 Projection;
    mat4 ViewProjection;
    mat4 Billboard;
} camera;

out vec3 v3Position;
out vec2 v2TexCoord;
out vec4 v4Color;

void main()
{
    // 1. Get the matrix for THIS patch
    mat4 model;
#ifdef MODERN_OPENGL_PATH
    model = modelMatrices[gl_BaseInstance];
#else
    model = u_matModel;
#endif

	gl_Position =  camera.ViewProjection * model * vec4(m_v3Position, 1.0);

	v3Position = m_v3Position;
	v2TexCoord = m_v2TexCoord;
	v4Color = m_v4Color;
}
