//
//  RNWater.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WATER_H__
#define __RAYNE_WATER_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNMesh.h"
#include "RNMaterial.h"
#include "RNRenderer.h"

namespace RN
{
	class Water : public SceneNode
	{
	public:
		RNAPI Water(Camera *cam, Texture *refract);
		RNAPI ~Water() override;
		
		RNAPI void SetTexture(Texture *texture);

		RNAPI void Update(float delta);
		
		RNAPI virtual bool IsVisibleInCamera(Camera *camera) override;
		RNAPI virtual void Render(Renderer *renderer, Camera *camera) override;
		
	private:
		void Initialize();
		
		Vector2 _size;
		Mesh *_mesh;
		Matrix _transform;
		class Material *_material;
		
		Camera *_camera;
		Camera *_reflection;
		Texture *_refraction;
		
		RNDeclareMeta(Water)
	};
}

#endif /* __RAYNE_WATER_H__ */
