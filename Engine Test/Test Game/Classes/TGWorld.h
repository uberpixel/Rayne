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
#include "TGFire.h"
#include "TGDebugDrawer.h"

#define TGSunTag   1
#define TGLightTag 2

namespace TG
{
	class World : public RN::World
	{
	public:
		World();
		~World();
		
		void LoadOnThread(RN::Thread *thread, RN::Deserializer *deserializer) override;
		
		void Update(float delta) override;
		void UpdateEditMode(float delta) override;
		
		void SetCutScene(const std::string &file);
		
	protected:
		void CreateCameras();
		void LoadLevelJSON(const std::string &file);
		
		void ToggleFrameCapturing();
		void RecordFrame();
		void HandleInputEvent(RN::Event *event);
		
		RN::Camera *_camera;
		RN::Camera *_refractCamera;
		RN::WeakRef<Sun> _sunLight;
		
		std::vector<RN::AABB> _obstacles;
		
	private:
		RN::PostProcessingPipeline *PPCreateBloomPipeline(RN::Camera *camera);
		RN::PostProcessingPipeline *PPCreateSSAOPipeline(RN::Camera *camera);
		RN::PostProcessingPipeline *PPCreateFXAAPipeline(RN::Camera *camera);
		RN::PostProcessingPipeline *PPCreateGodraysPipeline(RN::Camera *cam, RN::Texture *raysource);
		
		void PPToggleBloom();
		void PPToggleGodrays();
		void PPToggleSSAO();
		void PPToggleFXAA();
		
		bool _bloomActive;
		bool _ssaoActive;
		bool _godraysActive;
		bool _fxaaActive;
		
		RN::Camera *_waterCamera;
		RN::PostProcessingPipeline *_ssaoPipeline;
		RN::PostProcessingPipeline *_godraysPipeline;
		RN::PostProcessingPipeline *_bloomPipeline;
		RN::PostProcessingPipeline *_fxaaPipeline;
		
		float _exposure;
		float _whitepoint;
		
		bool _frameCapturing;
		size_t _frameCount;
		size_t _captureCount;
		
		DebugDrawer *_debugDrawer;
		CutScene *_cutScene;
		RN::GamepadDevice *_gamepad;
		RN::Array *_axises;
		
		RN::UI::Widget *_widget;
		RN::UI::Label *_label;
		
		RNDeclareMeta(World)
	};
}

#endif /* defined(__Game__TGWorld__) */
