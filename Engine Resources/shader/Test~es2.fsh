
precision mediump float;

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;

varying lowp vec4 color;
varying lowp vec2 texcoord;

void main()
{
	vec4 outcolor0 = texture2D(mTexture0, texcoord);
	vec4 outcolor1 = texture2D(mTexture1, texcoord);
	
	gl_FragColor = outcolor0 * outcolor1; //outcolor0 * color;
}
