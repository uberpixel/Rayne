//
//  RNDecal.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DECAL_H__
#define __RAYNE_DECAL_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNMesh.h"
#include "RNMaterial.h"
#include "RNRenderer.h"

namespace RN
{
	class Decal : public SceneNode
	{
	public:
		RNAPI Decal(bool tangents = true);
		RNAPI Decal(Texture *texture, bool tangents = true);
		RNAPI Decal(Texture *texture, const Vector3 &position, bool tangents = true);
		RNAPI Decal(const Decal *other);
		RNAPI Decal(Deserializer *deserializer);
		RNAPI ~Decal() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		RNAPI void SetTexture(Texture *texture, float scaleFactor = 0.1f);
		
		RNAPI void Update(float delta) override;
		RNAPI void UpdateEditMode(float delta) override;
		
		RNAPI Material *GetMaterial() const { return _material; }
		RNAPI Texture *GetTexture() const;
		
		RNAPI void SetAngle(float angle);
		RNAPI float GetAngle() const { return _angle; };
		
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode) override;
		
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
	private:
		void Initialize();
		void UpdateMesh();
		
		bool _dirty;
		
		Observable<float, Decal> _angle;
		float _angleCos;
		
		Mesh *_mesh;
		Matrix _transform;
		Material *_material;
		
		bool _tangents;
		
		RNDeclareMeta(Decal)
	};
}

#endif /* __RAYNE_DECAL_H__ */
