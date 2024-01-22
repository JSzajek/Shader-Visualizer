#shader vertex
#version 420

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoords;

layout(location = 0) out vec2 TexCoords;
layout(location = 1) out vec2 PixCoord;

void main()
{
	TexCoords = a_TexCoords;
	PixCoord = a_TexCoords * u_Viewport.xy;

	gl_Position = vec4(a_Position.x, a_Position.y, 0.0f, 1.0f);
}

#shader fragment
#version 420
			
layout(location = 0) in vec2 TexCoords;
layout(location = 1) in vec2 PixCoord;

layout(location = 0) out vec4 Color;
layout(location = 1) out vec4 BloomColor;

#define UVS TexCoords
#define PIXCOORD PixCoord

#define RESOLUTION (u_Viewport.xy)
#define TIME u_Time

void PixelProcess(out vec4 color);

void main()
{
	PixelProcess(Color);

	float brightness = dot(Color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BloomColor = vec4(Color.rgb, 1.0);
    else
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
}
