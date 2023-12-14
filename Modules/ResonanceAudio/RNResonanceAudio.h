//
//  RNResonanceAudio.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudio_H_
#define __RAYNE_ResonanceAudio_H_

#include <Rayne.h>

#if defined(RN_BUILD_RESONANCE_AUDIO)
	#define RAAPI RN_EXPORT
#else
	#define RAAPI RN_IMPORT
#endif

#endif /* __RAYNE_ResonanceAudio_H_ */
