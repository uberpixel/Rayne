#version 150
precision highp float;

uniform sampler2D targetmap;

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	fragColor0 = texture(targetmap, texcoord);
}
