//
//  RNBillboard.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		RNAPI Billboard(Texture *texture);
		RNAPI Billboard(Texture *texture, const Vector3 &position);
		RNAPI ~Billboard() override;
		
		RNAPI void SetTexture(Texture *texture, float scaleFactor = 0.1f);
		
		RNAPI Material *GetMaterial() const { return _material; }
		RNAPI Texture *GetTexture() const;
		RNAPI const Vector2 &GetSize() const { return _size; }
		
		
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode) override;
		
	private:
		void Initialize();
		
		Vector2 _size;
		Mesh *_mesh;
		Matrix _transform;
		Material *_material;
		
		RNDeclareMetaWithTraits(Billboard, SceneNode, MetaClassTraitCronstructable, MetaClassTraitCopyable)
	};
}

#endif /* __RAYNE_BILLBOARD_H__ */
