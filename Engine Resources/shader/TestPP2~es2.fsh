
precision mediump float;

uniform sampler2D targetmap;
varying lowp vec2 texcoord;

void main()
{
	vec4 color = texture2D(targetmap, texcoord);
	float luminosity = (0.2125 * color.r) + (0.7154 * color.g) + (0.0721 * color.b);
	
	gl_FragColor = vec4(luminosity, luminosity, luminosity, color.a);
}
