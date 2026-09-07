//
//  RNLightClusterSceneAttachment.cpp
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLightClusterSceneAttachment.h"
#include "RNCamera.h"
#include "RNLightClusterPassSnapshot.h"
#include "RNLightClusterRendererAttachment.h"
#include "RNLightManager.h"
#include "RNScene.h"

namespace RN
{
	RNDefineMeta(LightClusterSceneAttachment, SceneAttachment)

	void LightClusterSceneAttachment::AttachToScene(Scene *scene)
	{
		RN_ASSERT(scene, "Scene mustn't be NULL");

		LightClusterSceneAttachment *attachment = new LightClusterSceneAttachment();
		scene->AddAttachment(attachment);
		SafeRelease(attachment);
	}

	LightClusterSceneAttachment::LightClusterSceneAttachment()
	{}

	LightClusterSceneAttachment::~LightClusterSceneAttachment()
	{}

	void LightClusterSceneAttachment::DidAttachToRenderer(Renderer *renderer)
	{
		if(renderer->HasAttachment(LightClusterRendererAttachment::GetMetaClass()))
			return;

		LightClusterRendererAttachment *attachment = new LightClusterRendererAttachment();
		renderer->AddAttachment(attachment);
		SafeRelease(attachment);
	}

	void LightClusterSceneAttachment::SubmitCameraPassAttachmentSnapshots(Renderer *renderer, Camera *camera, const SceneCameraPassContext &context)
	{
		RN_ASSERT(renderer, "Renderer mustn't be NULL");

		if(!camera)
			return;

		LightManager *lightManager = camera->GetLightManager();
		if(!lightManager)
			return;

		LightClusterPassSnapshot *snapshot = new LightClusterPassSnapshot(lightManager, lightManager->CaptureBuildInput(camera, context.visibleLights));
		renderer->SubmitCameraPassAttachmentSnapshot(snapshot);
		SafeRelease(snapshot);
	}
} // namespace RN
