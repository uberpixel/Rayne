//
//  RNOggAssetLoader.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OGGASSETLOADER_H_
#define __RAYNE_OGGASSETLOADER_H_

#include "RNOgg.h"

namespace RN
{
	class OggAssetLoader : public AssetLoader
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		Asset *Load(File *file, const LoadOptions &options) override;

	private:
		OggAssetLoader(const Config &config);

		RNDeclareMetaAPI(OggAssetLoader, OGGAPI)
	};
}


#endif /* __RAYNE_ASSIMPASSETLOADER_H_ */
