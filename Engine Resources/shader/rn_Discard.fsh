//
//  rn_Discard.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_DISCARD_FSH
#define RN_DISCARD_FSH

#ifdef RN_DISCARD

uniform float discardThreshold;

#define rn_Discard(color) \
	if(color.a < discardThreshold) \
		discard

#else
#define rn_Discard(color)
#endif

#endif
