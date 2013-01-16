//
//  TestPP.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision mediump float;

uniform sampler2D targetmap;
varying lowp vec2 texcoord;

void main()
{
	vec2 tex;
	vec2 pixelation = vec2(100.0, 100.0);
	
	tex.x = sign(texcoord.x * pixelation.x) * floor(abs(texcoord.x * pixelation.x) + 0.5);
	tex.x = tex.x / pixelation.x;
	
	tex.y = sign(texcoord.y * pixelation.y) * floor(abs(texcoord.y * pixelation.y) + 0.5);
	tex.y = tex.y / pixelation.y;
	
	gl_FragColor = texture2D(targetmap, tex);
}
