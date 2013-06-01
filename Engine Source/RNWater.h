//
//  RNWater.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		Water(Camera *cam, Texture *refract);
		virtual ~Water();
		
		void SetTexture(Texture *texture);
		
		Material *Material() const { return _material; }
		
		void Update(float delta);
		bool CanUpdate(FrameID frameid);
		
		virtual bool IsVisibleInCamera(Camera *camera) override;
		virtual void Render(Renderer *renderer, Camera *camera) override;
		
	private:
		void Initialize();
		
		Vector2 _size;
		Mesh *_mesh;
		Matrix _transform;
		class Material *_material;
		
		Camera *_camera;
		Camera *_reflection;
		Texture *_refraction;
		
		RNDefineConstructorlessMeta(Water, SceneNode)
	};
}

#endif /* __RAYNE_WATER_H__ */
