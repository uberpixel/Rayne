//
//  RNAssimpAssetLoader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSIMPASSETLOADER_H_
#define __RAYNE_ASSIMPASSETLOADER_H_

#include "RNAssimp.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace RN
{
	class AssimpAssetLoader : public AssetLoader
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		Asset *Load(File *file, MetaClass *meta, Dictionary *settings) override;

	private:
		AssimpAssetLoader(const Config &config);

		void LoadAssimpLODStage(const String *filepath, const aiScene *scene, Model::LODStage *stage);
		std::pair<Mesh *, Material *> LoadAssimpMeshGroup(const String *filepath, const aiScene *scene, size_t index);

		Mesh *LoadAssimpMesh(const aiScene *scene, size_t index);
		std::shared_future<StrongRef<Asset>> LoadAssimpTexture(aiMaterial *material, const String *path, aiTextureType aitexturetype, uint8 index);

		RNDeclareMetaAPI(AssimpAssetLoader, ASAPI)
	};
}


#endif /* __RAYNE_ASSIMPASSETLOADER_H_ */
