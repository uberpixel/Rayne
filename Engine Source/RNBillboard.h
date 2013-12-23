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
		RNAPI Billboard();
		RNAPI ~Billboard() override;
		
		RNAPI void SetTexture(Texture *texture);
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode) override;
		
		RNAPI Material *GetMaterial() const { return _material; }
		
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
