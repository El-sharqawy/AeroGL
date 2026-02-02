
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_ARB_bindless_texture : require

layout (location = 0) in vec3 m_v3Position;
layout (location = 1) in vec3 m_v3Normals;
layout (location = 2) in vec2 m_v2TexCoord;
layout (location = 3) in vec4 m_v4Color;

#if __VERSION__ >= 420
#define MODERN_OPENGL_PATH
#endif

#ifdef MODERN_OPENGL_PATH
// This is required to tell the compiler how to treat uvec2 as a sampler
layout(bindless_sampler) uniform sampler2D;

struct PatchData
{
    mat4 modelMatrix;
    uvec2 heightMapHandle; // 64-bit handle stored as two 32-bit uints
    float heightScale;
    uvec2 terrainCoords;
};

layout(std430, binding = 0) buffer TerrainData
{
    PatchData patches[];
};
#else
uniform mat4 u_matModel;
uniform int u_vertex_DrawID;
#endif

uniform int heightMapSize;

// Camera UBO (works on OpenGL 3.1+)
layout (std140, binding = 0) uniform CameraData
{
    mat4 View;
    mat4 Projection;
    mat4 ViewProjection;
    mat4 Billboard;
} camera;

out vec3 v3Position;
out vec3 v3Normals;
out vec2 v2TexCoord;
out vec4 v4Color;

out flat int vertex_DrawID;

void main()
{
    // 1. Get the matrix for THIS patch
    mat4 model;
    int index = 0;

    // gl_InstanceID starts from the baseInstance you set in Indirect Command

#ifdef MODERN_OPENGL_PATH
    index = gl_BaseInstanceARB; 
    PatchData currentPatch = patches[index];
    model = currentPatch.modelMatrix;
    v3Position = vec3(model * vec4(m_v3Position, 1.0));

    vec2 heightmapUV;
    heightmapUV.x = mod(v3Position.x, heightMapSize) / heightMapSize;
    heightmapUV.y = mod(v3Position.z, heightMapSize) / heightMapSize;

    // Sample the heightmap using the calculated global coordinates
    sampler2D hMap = sampler2D(currentPatch.heightMapHandle);
    float h = texture(hMap, heightmapUV).r;

    float scale = currentPatch.heightScale;
#else
    index = u_vertex_DrawID;
    model = u_matModel;
    v3Position = vec3(model * vec4(m_v3Position, 1.0));


    float h = 0.0; // Legacy fallback
    float scale = 0.0;
#endif

    // Apply displacement
    vec3 displacedPos = m_v3Position;
    displacedPos.y += h * currentPatch.heightScale;

    // standard MVP transformation
    gl_Position = camera.ViewProjection * model * vec4(displacedPos, 1.0);

    // Pass data to Fragment Shader
    v3Normals = m_v3Normals;
	v2TexCoord = m_v2TexCoord;
	v4Color = m_v4Color;

    vertex_DrawID = index;
}
