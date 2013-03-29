

#ifdef RN_DISCARD

uniform float discardThreshold;
#define rn_Discard(color) \
	 if(color.a < discardThreshold) \
	 	discard

#else

#define rn_Discard(color)

#endif
