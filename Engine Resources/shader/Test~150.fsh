#version 150
precision highp float;

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;
uniform float time;

in vec2 texcoord;

out vec4 fragColor0;

void main()
{
	vec4 outcolor0 = texture(mTexture0, texcoord);
//	vec4 outcolor1 = texture(mTexture1, texcoord);
	
	fragColor0 = outcolor0;//*outcolor1;//vec4(cos(time*10.0+texcoord.x*10.0)*0.5+0.5, cos(time*20.0+texcoord.x*20.0)*0.5+0.5, sin(time*10.0+texcoord.y*20.0)*0.5+0.5, 1.0);//outcolor0 * outcolor1;
}
