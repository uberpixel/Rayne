//
//  TGForestWorld.h
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Test_Game__TGForestWorld__
#define __Test_Game__TGForestWorld__

#include <Rayne.h>
#include "TGWorld.h"

#define TGForestTreeCount  10
#define TGForestGrassCount 4
#define TGForestReedCount  1

namespace TG
{
	class ForestWorld : public World
	{
	public:
		ForestWorld();
		void LoadOnThread(RN::Thread *thread, RN::Deserializer *deserializer) override;
		void SaveOnThread(RN::Thread *thread, RN::Serializer *serializer) override;
		
	private:
		void LoadBlendAndHeightmap();
		void LoadTreeModels();
		void LoadGrassModels();
		
		size_t IndexForPosition(const RN::Vector3 &position);
		bool PositionBlocked(const RN::Vector3 &position);
		float GetGroundHeight(const RN::Vector3 &position);
		
		std::vector<float> _heightmap;
		std::vector<RN::Color> _blendmap;
		std::vector<RN::Color> _flowermap;
		float _heightBase;
		float _heightExtent;
		
		RN::Entity *_ground;
		
		RN::Model *_trees[TGForestTreeCount];
		RN::Model *_grass[TGForestGrassCount];
		RN::Model *_reeds[TGForestReedCount];
		
		RNDeclareMeta(ForestWorld)
	};
}

#endif /* defined(__Test_Game__TGForestWorld__) */