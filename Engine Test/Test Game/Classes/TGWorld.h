//
//  TGWorld.h
//  Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Game__TGWorld__
#define __Game__TGWorld__

#include <Rayne.h>

#include "TGSun.h"
#include "TGCutScene.h"

namespace TG
{
	class World : public RN::World
	{
	public:
		World();
		~World();
		
		void Update(float delta) override;
		
	protected:
		void CreateCameras();
		void LoadLevelJSON(const std::string &file);
		
		void ToggleFrameCapturing();
		void RecordFrame();
		void HandleInputEvent(RN::Event *event);
		
		RN::Camera *_camera;
		RN::Camera *_refractCamera;
		Sun *_sunLight;
		
		std::vector<RN::AABB> _obstacles;
		
	private:
		RN::PostProcessingPipeline *PPCreateBloomPipeline(RN::Camera *camera);
		RN::PostProcessingPipeline *PPCreateSSAOPipeline(RN::Camera *camera);
		RN::PostProcessingPipeline *PPCreateFXAAPipeline(RN::Camera *camera);
		
		void PPToggleBloom();
		void PPToggleSSAO();
		void PPToggleFXAA();
		
		bool _bloomActive;
		bool _ssaoActive;
		bool _fxaaActive;
		
		RN::Camera *_waterCamera;
		RN::PostProcessingPipeline *_ssaoPipeline;
		RN::PostProcessingPipeline *_bloomPipeline;
		RN::PostProcessingPipeline *_fxaaPipeline;
		
		float _exposure;
		float _whitepoint;
		
		bool _frameCapturing;
		size_t _frameCount;
	};
}

#endif /* defined(__Game__TGWorld__) */
