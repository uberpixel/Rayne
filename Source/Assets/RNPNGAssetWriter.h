//
//  RNPNGAssetWriter.h
//  Rayne
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PNGASSETWRITER_H_
#define __RAYNE_PNGASSETWRITER_H_

#include "../Base/RNBase.h"
#include "../Rendering/RNTexture.h"

namespace RN
{
	class PNGAssetWriter
	{
	public:
		RNAPI static void Encode(Texture *texture, std::function<void(RN::Data *pngData)> callback, bool interlace = true);
		RNAPI static RN::Data *Encode(Bitmap *bitmap, bool interlace = true);
		RNAPI static bool Write(Texture *texture, const String *filename, bool interlace = true);
	};
}


#endif /* __RAYNE_PNGASSETWRITER_H_ */
