//
//  RNAssimpAssetLoader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSIMPASSETLOADER_H_
#define __RAYNE_ASSIMPASSETLOADER_H_

#include <Rayne.h>

class aiScene;

namespace RN
{
	class AssimpAssetLoader : public AssetLoader
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		Asset *Load(File *file, MetaClass *meta, Dictionary *settings) override;

	private:
		AssimpAssetLoader(const Config &config);

		Mesh *LoadAssimpMesh(const aiScene *scene, size_t index);

		RNDeclareMeta(AssimpAssetLoader)
	};
}


#endif /* __RAYNE_ASSIMPASSETLOADER_H_ */
