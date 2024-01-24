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

layout(binding = 0) uniform sampler2D albedoTexture;
layout(binding = 1) uniform sampler2D bloomTexture;

void main()
{
	vec3 hdrColor = texture(albedoTexture, TexCoords).rgb;
	vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;
	
	hdrColor += bloomColor; // additive blending
	
	// tone mapping
	vec3 result = vec3(1.0) - exp(-hdrColor * u_Exposure);
	
	// also gamma correct while we're at it       
	result = pow(result, vec3(1.0 / u_GammaAdjustment.x));
	
	Color = vec4(result, 1.0);
}
