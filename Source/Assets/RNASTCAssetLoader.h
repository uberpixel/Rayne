//
//  RNASTCAssetLoader.h
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASTCASSETLOADER_H_
#define __RAYNE_ASTCASSETLOADER_H_

#include "../Base/RNBase.h"
#include "RNAssetLoader.h"

namespace RN
{
	class ASTCAssetLoader : public AssetLoader
	{
	public:
		static void Register();

		Asset *Load(File *file, const LoadOptions &options) override;

	private:
		struct ASTCFormatHeader
		{
			uint8_t magic[4];
			uint8_t blockdim_x;
			uint8_t blockdim_y;
			uint8_t blockdim_z ;
			uint8_t xsize[3];
			uint8_t ysize[3];
			uint8_t zsize[3];
		};
		
		ASTCAssetLoader(const Config &config);

		__RNDeclareMetaInternal(ASTCAssetLoader)
	};
}


#endif /* __RAYNE_ASTCASSETLOADER_H_ */
