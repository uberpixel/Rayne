//
//  RNResonanceAudioInternals.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudioINTERNALS_H_
#define __RAYNE_ResonanceAudioINTERNALS_H_

#include "RNResonanceAudio.h"

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#include "miniaudio.h"

namespace RN
{
	struct ResonanceAudioSystemMiniAudioInternals
	{
		ma_context context;
		ma_device outputDevice;
		ma_device inputDevice;
	};
}

#endif /* defined(__RAYNE_ResonanceAudioINTERNALS_H_) */
