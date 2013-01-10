
precision mediump float;

uniform sampler2D targetmap;
varying lowp vec2 texcoord;

void main()
{
	gl_FragColor = texture2D(targetmap, texcoord);
}
