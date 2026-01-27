#version 460 core

// Required for your MakeTextureResident logic
#ifdef USE_BINDLESS
    #extension GL_ARB_bindless_texture : require
#endif

layout (location = 0) out vec4 v4FragColor;

in vec3 v3Position;
in vec3 v3Normals;
in vec2 v2TexCoord;
in vec4 v4Color;

in flat int vertex_DrawID;

// If you are NOT using bindless, remove 'layout(bindless_sampler)'
// 2. Conditionally define the sampler
#ifdef USE_BINDLESS
    layout(bindless_sampler) uniform sampler2D u_DiffuseTexture;
#else
    uniform sampler2D u_DiffuseTexture; // Standard 32-bit slot sampler
#endif

uniform bool u_bUseDiffuseTexture;
uniform vec3 u_lightPos;
uniform vec3 u_lightColor;

void main()
{
	if (vertex_DrawID == 0 || vertex_DrawID == 1) // Sun and Axis color, remain the same
    {
        float gamma = 2.2;
        v4FragColor = v4Color; 
        v4FragColor.rgb = pow(v4FragColor.rgb, vec3(1.0/gamma));
        return;
    }

	float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * u_lightColor;

    vec3 result = ambient * v4Color.xyz;

	vec3 norm = normalize(v3Normals);
	vec3 lightDir = normalize(u_lightPos - v3Position);  
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * u_lightColor;

	result = (ambient + diffuse) * v4Color.xyz;

    float gamma = 2.2;
    v4FragColor = vec4(result, 1.0);
    v4FragColor.rgb = pow(v4FragColor.rgb, vec3(1.0/gamma));
}
