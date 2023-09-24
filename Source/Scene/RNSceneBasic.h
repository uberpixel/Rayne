//
//  RNSceneBasic.h
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENEBASIC_H__
#define __RAYNE_SCENEBASIC_H__

#include "RNScene.h"

namespace RN
{
	class SceneBasic : public Scene
	{
	public:
		RNAPI ~SceneBasic();

		RNAPI void AddNode(SceneNode *node) override;
		RNAPI void RemoveNode(SceneNode *node) override;

	protected:
		RNAPI SceneBasic();

		RNAPI void Update(float delta) override;
		RNAPI void Render(Renderer *renderer) override;
		
        RNAPI void FlushAdditionQueue();
		RNAPI void FlushDeletionQueue();
		
		RNAPI void AddRenderNode(SceneNode *node);
		RNAPI void RemoveRenderNode(SceneNode *node);

		//Should probably be private, but this makes it easy to visualize
		RN::uint16 _occlusionDepthBufferWidth;
		RN::uint16 _occlusionDepthBufferHeight;
		float *_occlusionDepthBuffer;

	private:
		void RasterizeClipSpaceTriangle(Vector4 A, Vector4 B, Vector4 C);
		void RasterizeMesh(const Matrix &matModelViewProj, Mesh *mesh);
		bool TestBoundingBox(const Matrix &matViewProj, const AABB &aabb, const Vector2 &screenPixelSize);
		
		IntrusiveList<SceneNode> _updateNodes[3];
		IntrusiveList<SceneNode> _renderNodes;
		IntrusiveList<Light> _lights;
		IntrusiveList<Camera> _cameras;
		Array *_nodesToRemove;
        Array *_nodesToAdd;

		__RNDeclareMetaInternal(SceneBasic)
	};
}


#endif /* __RAYNE_SCENEBASIC_H__ */
