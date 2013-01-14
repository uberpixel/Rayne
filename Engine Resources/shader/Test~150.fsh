#version 150
precision highp float;

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;
uniform float time;

in vec4 color;
in vec2 texcoord;

out vec4 fragColor0;

void main()
{
	vec4 outcolor0 = texture(mTexture0, texcoord);
	vec4 outcolor1 = texture(mTexture1, texcoord);
	
	fragColor0 = outcolor0 * outcolor1;
	fragColor0.a = sin(time);
}
