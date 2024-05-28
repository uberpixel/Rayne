//
//  RNVRCompositorLayer.h
//  Rayne-VR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VRCOMPOSITORLAYER_H_
#define __RAYNE_VRCOMPOSITORLAYER_H_

#include "RNVR.h"

namespace RN
{
	class VRCompositorLayer : public Object
	{
	public:

		enum Type
		{
			TypeProjectionView,
			TypeQuad,
			TypePassthrough
		};

		RNVRAPI ~VRCompositorLayer();

		RNVRAPI void SetPosition(Vector3 position) { _position = position; }
		RNVRAPI void SetRotation(Quaternion rotation) { _rotation = rotation; }
		RNVRAPI void SetScale(Vector2 scale) { _scale = scale; }

		RNVRAPI const Vector3 &GetPosition() { return _position; }
		RNVRAPI const Quaternion &GetRotation() { return _rotation; }
		RNVRAPI const Vector2 &GetScale() { return _scale; }
		
		RNVRAPI virtual void SetActive(bool active) { }
		RNVRAPI virtual void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic) { }
		
		RNVRAPI virtual Vector2 GetSize() const = 0;
		RNVRAPI virtual size_t GetImageCount() const = 0;

		RNVRAPI virtual Framebuffer *GetFramebuffer() const = 0;
		RNVRAPI virtual Framebuffer *GetFramebuffer(uint8 image) const { return nullptr; }

		RNVRAPI Type GetType() const { return _type; }

	protected:
		RNVRAPI VRCompositorLayer(Type type);

		Type _type;
		Vector3 _position;
		Quaternion _rotation;
		RN::Vector2 _scale;

	private:
		RNDeclareMetaAPI(VRCompositorLayer, RNVRAPI)
	};
}


#endif /* __RAYNE_VRCOMPOSITORLAYER_H_ */
