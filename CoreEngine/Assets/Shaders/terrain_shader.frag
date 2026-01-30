
// Required for your MakeTextureResident logic
#ifdef USE_BINDLESS
    #extension GL_ARB_bindless_texture : require
#endif

layout (location = 0) out vec4 v4FragColor;

in vec3 v3Position;
in vec2 v2TexCoord;
in vec3 v3Normals;
in vec4 v4Color;

// If you are NOT using bindless, remove 'layout(bindless_sampler)'
// 2. Conditionally define the sampler
#ifdef USE_BINDLESS
    layout(bindless_sampler) uniform sampler2D u_DiffuseTexture;
#else
    uniform sampler2D u_DiffuseTexture; // Standard 32-bit slot sampler
#endif

uniform bool u_bUseDiffuseTexture;

void main()
{
	// If you see a Red/Green gradient, your UVs are correct.
    // v4FragColor = vec4(v2TexCoord, 0.0, 1.0);
	vec4 texColor = v4Color;
	if (u_bUseDiffuseTexture)
	{
		// Samples the texture using the UV coordinates from the vertex shader
		texColor = texture(u_DiffuseTexture, v2TexCoord); // No 'vec4' here
	}	
	v4FragColor = texColor;
}
