//
//  rn_Color2.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform mat4 matProjViewModel;

attribute vec3 vertPosition;
attribute vec4 vertColor0;
attribute vec4 vertColor1;

varying vec4 outColor0;
varying vec4 outColor1;

void main()
{
	outColor0 = vertColor0;
	outColor1 = vertColor1;

	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}
