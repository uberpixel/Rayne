#version 150
precision highp float;

uniform sampler2D targetmap;

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	vec4 color = texture(targetmap, texcoord);
	fragColor0 = color; //vec4(0.0, 1.0, 0.0, 1.0) * color; //texture(targetmap, texcoord);
}
