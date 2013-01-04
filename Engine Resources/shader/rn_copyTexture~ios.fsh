
precision mediump float;

uniform sampler2D mTexture0;
varying vec2 texcoord;

void main()
{
	gl_FragColor = texture2D(mTexture0, texcoord);
}
