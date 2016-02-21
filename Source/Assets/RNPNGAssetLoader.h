//
//  RNPNGAssetLoader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PNGASSETLOADER_H_
#define __RAYNE_PNGASSETLOADER_H_

#include "../Base/RNBase.h"
#include "RNAssetLoader.h"

namespace RN
{
	class PNGAssetLoader : public AssetLoader
	{
	public:
		static void Register();

		Asset *Load(File *file, MetaClass *meta, Dictionary *settings) override;

	private:
		PNGAssetLoader(const Config &config);

		__RNDeclareMetaInternal(PNGAssetLoader)
	};
}


#endif /* __RAYNE_PNGASSETLOADER_H_ */
