//
//  RNBillboard.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BILLBOARD_H__
#define __RAYNE_BILLBOARD_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNMesh.h"
#include "RNMaterial.h"
#include "RNRenderer.h"

namespace RN
{
	class Billboard : public SceneNode
	{
	public:
		Billboard();
		virtual ~Billboard();
		
		void SetTexture(Texture *texture);
		
		Material *Material() const { return _material; }
		
		virtual bool IsVisibleInCamera(Camera *camera) override;
		virtual void Render(Renderer *renderer, Camera *camera) override;
		
	private:
		void Initialize();
		
		Vector2 _size;
		Mesh *_mesh;
		Matrix _transform;
		class Material *_material;
	};
}

#endif /* __RAYNE_BILLBOARD_H__ */
