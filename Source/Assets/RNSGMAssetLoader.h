//
//  RNSGMAssetLoader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SGMASSETLOADER_H_
#define __RAYNE_SGMASSETLOADER_H_

#include "../Base/RNBase.h"
#include "../Rendering/RNModel.h"
#include "RNAssetLoader.h"

namespace RN
{
	class SGMAssetLoader : public AssetLoader
	{
	public:
		static void Register();

		Asset *Load(File *file, MetaClass *meta, Dictionary *settings) override;
		bool SupportsLoadingFile(File *file) const override;

	private:
		SGMAssetLoader(const Config &config);

		void LoadLODStage(File *file, Model::LODStage *stage);

		RNDeclareMeta(SGMAssetLoader)
	};
}


#endif /* __RAYNE_SGMASSETLOADER_H_ */
