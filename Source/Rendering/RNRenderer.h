//
//  RNRenderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_RENDERER_H_
#define __RAYNE_RENDERER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNWeakStorage.h"
#include "../System/RNScreen.h"
#include "../Scene/RNCamera.h"
#include "../Math/RNVector.h"
#include "../Math/RNMatrix.h"
#include "../Math/RNQuaternion.h"
#include "../Math/RNColor.h"
#include "RNWindow.h"
#include "RNGPUBuffer.h"
#include "RNShaderLibrary.h"
#include "RNTexture.h"
#include "RNFramebuffer.h"
#include "RNRendererTypes.h"
#include "RNMesh.h"
#include "RNMaterial.h"

namespace RN
{
	struct Drawable
	{
		Drawable()
		{
			dirty = true;
			mesh = nullptr;
			material = nullptr;
			count = 1;
		}
		virtual ~Drawable()
		{}

		void Update(Mesh *tmesh, Material *tmaterial, const SceneNode *node)
		{
			if(mesh != tmesh)
			{
				dirty = true;
				mesh = tmesh;
			}
			if(material != tmaterial)
			{
				dirty = true;
				material = tmaterial;
			}

			if(node)
				Update(node);
		}
		virtual void Update(const SceneNode *node)
		{
			modelMatrix = node->GetWorldTransform();
			inverseModelMatrix = modelMatrix.GetInverse();
		}

		Mesh *mesh;
		Material *material;
		Matrix modelMatrix;
		Matrix inverseModelMatrix;
		size_t count;
		bool dirty;
	};

	class RendererDescriptor;
	class RenderingDevice;

	class Renderer : public Object
	{
	public:
		RNAPI static Renderer *GetActiveRenderer();

		RNAPI virtual Window *CreateAWindow(const Vector2 &size, Screen *screen) = 0;
		RNAPI virtual Window *GetMainWindow() = 0;

		RNAPI void Activate();
		RNAPI virtual void Deactivate();

		RNAPI virtual void RenderIntoWindow(Window *window, Function &&function) = 0;
		RNAPI virtual void RenderIntoCamera(Camera *camera, Function &&function) = 0;

		RNAPI virtual bool SupportsTextureFormat(const String *format) const = 0;
		RNAPI virtual bool SupportsDrawMode(DrawMode mode) const = 0;

		RNAPI virtual size_t GetAlignmentForType(PrimitiveType type) const = 0;
		RNAPI virtual size_t GetSizeForType(PrimitiveType type) const = 0;
		RNAPI virtual const String *GetTextureFormatName(const Texture::Format format) const = 0;

		RNAPI virtual GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions options) = 0;
		RNAPI virtual GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options) = 0;

		RNAPI virtual ShaderLibrary *CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options) = 0;
		RNAPI virtual ShaderLibrary *CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options) = 0;

		RNAPI virtual ShaderProgram *GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup) = 0;

		RNAPI virtual Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) = 0;

		RNAPI virtual Drawable *CreateDrawable() = 0;
		RNAPI virtual void SubmitDrawable(Drawable *drawable) = 0;

		RendererDescriptor *GetDescriptor() const { return _descriptor; }
		RenderingDevice *GetDevice() const { return _device; }

	protected:
		RNAPI Renderer(RendererDescriptor *descriptor, RenderingDevice *device);

	private:
		RenderingDevice *_device;
		RendererDescriptor *_descriptor;

		RNDeclareMeta(Renderer)
	};

	RNExceptionType(ShaderCompilation)
}


#endif /* __RAYNE_RENDERER_H_ */
