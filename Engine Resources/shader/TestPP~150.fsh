#version 150
precision highp float;

uniform sampler2D targetmap;

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	vec2 tex;
	vec2 pixelation = vec2(100.0, 100.0);
	
	tex.x = sign(texcoord.x * pixelation.x) * floor(abs(texcoord.x * pixelation.x) + 0.5);
	tex.x = tex.x / pixelation.x;
	
	tex.y = sign(texcoord.y * pixelation.y) * floor(abs(texcoord.y * pixelation.y) + 0.5);
	tex.y = tex.y / pixelation.y;
	
	fragColor0 = texture(targetmap, tex);
}
