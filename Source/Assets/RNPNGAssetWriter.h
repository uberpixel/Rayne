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
		RNAPI static void Encode(Texture *texture, std::function<void(RN::Data *pngData)> callback);
		RNAPI static bool Write(Texture *texture, const String *filename);
	};
}


#endif /* __RAYNE_PNGASSETWRITER_H_ */
