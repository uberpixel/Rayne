//
//  RNglTFAssetLoader.h
//  Rayne
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_GLTFASSETLOADER_H_
#define __RAYNE_GLTFASSETLOADER_H_

#include "RNglTF.h"
#include "tiny_gltf.h"

namespace RN
{
	class GlTFAssetLoader : public AssetLoader
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		Asset *Load(File *file, const LoadOptions &options) override;

	private:
		GlTFAssetLoader(const Config &config);
		
		void LoadGLTFLODStage(tinygltf::Model &gltfModel, Model::LODStage *stage, const LoadOptions &options);
		std::pair<Mesh *, Material *> LoadGLTFMeshGroup(tinygltf::Model &gltfModel, tinygltf::Mesh &gltfMesh, const LoadOptions &options);
		Mesh *LoadGLTFMesh(tinygltf::Model &gltfModel, tinygltf::Mesh &gltfMesh);

		RNDeclareMetaAPI(GlTFAssetLoader, TFAPI)
	};
}


#endif /* __RAYNE_GLTFASSETLOADER_H_ */
