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

#if defined(RN_BUILD_ResonanceAudio)
	#define OAAPI RN_EXPORT
#else
	#define OAAPI RN_IMPORT
#endif

#endif /* __RAYNE_ResonanceAudio_H_ */
