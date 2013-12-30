//
//  rn_GammaCorrection.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifdef RN_GAMMA_CORRECTION
	vec3 rn_GammaCorrection(vec3 color)
	{
		return pow(color, vec3(0.454545455));
	}
#else
	vec3 rn_GammaCorrection(vec3 color)
	{
		return color;
	}
#endif
