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
		~Billboard() override;
		
		void SetTexture(Texture *texture);
		void Render(Renderer *renderer, Camera *camera) override;
		Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode) override;
		
		Material *GetMaterial() const { return _material; }
		
	private:
		void Initialize();
		
		Vector2 _size;
		Mesh *_mesh;
		Matrix _transform;
		Material *_material;
		
		RNDefineMetaWithTraits(Billboard, SceneNode, MetaClassTraitCronstructable);
	};
}

#endif /* __RAYNE_BILLBOARD_H__ */
