#shader vertex
#version 420

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoords;

layout(location = 0) out vec2 TexCoords;

void main()
{
	TexCoords = a_TexCoords;

	gl_Position = vec4(a_Position.x, a_Position.y, 0.0f, 1.0f);
}

#shader fragment
#version 420
			
layout(location = 0) in vec2 TexCoords;

layout(location = 0) out vec4 Color;

layout(binding = 0) uniform sampler2D sampleTexture;

uniform bool horizontal;
uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main()
{
	vec2 tex_offset = 1.0 / textureSize(sampleTexture, 0); // gets size of single texel
    vec3 result = texture(sampleTexture, TexCoords).rgb * weight[0];
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
           result += texture(sampleTexture, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
           result += texture(sampleTexture, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(sampleTexture, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(sampleTexture, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
	Color = vec4(result, 1.0);
}
