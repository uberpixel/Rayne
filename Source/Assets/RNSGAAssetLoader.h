//
//  RNSGAAssetLoader.h
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SGAASSETLOADER_H__
#define __RAYNE_SGAASSETLOADER_H__

#include "../Base/RNBase.h"
#include "../Rendering/RNSkeleton.h"
#include "RNAssetLoader.h"

namespace RN
{
	class SGAAssetLoader : public AssetLoader
	{
	public:
		static void Register();
		
		Asset *Load(File *file, const LoadOptions &options) override;
		bool SupportsLoadingFile(File *file) const override;
		
	private:
		SGAAssetLoader(const Config &config);
		
		__RNDeclareMetaInternal(SGAAssetLoader)
	};
}

#endif /* __RAYNE_SGAASSETLOADER_H__ */
