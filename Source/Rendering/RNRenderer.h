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
#include "RNSkeleton.h"

namespace RN
{
	struct Drawable
	{
		Drawable()
		{
			mesh = nullptr;
			material = nullptr;
			skeleton = nullptr;
			count = 1;
		}
		virtual ~Drawable()
		{}

		void Update(Mesh *tmesh, Material *tmaterial, Skeleton *tskeleton, const SceneNode *node)
		{
			if(mesh != tmesh)
			{
				MakeDirty();
				mesh = tmesh;
			}
			if(material != tmaterial)
			{
				MakeDirty();
				material = tmaterial;
			}
			if(skeleton != tskeleton)
			{
				MakeDirty();
				skeleton = tskeleton;
			}

			if(node)
				Update(node);
		}
		virtual void Update(const SceneNode *node)
		{
            if(!node)
            {
                modelMatrix = Matrix();
                inverseModelMatrix = Matrix();
            }
            else
            {
                modelMatrix = node->GetWorldTransform();
                inverseModelMatrix = modelMatrix.GetInverse();
            }
		}
		virtual void MakeDirty(){}

		Mesh *mesh;
		Material *material;
		Skeleton *skeleton;
		Matrix modelMatrix;
		Matrix inverseModelMatrix;
		size_t count;
	};

	class RendererDescriptor;
	class RenderingDevice;
	class Light;

	class Renderer : public Object
	{
	public:
		RNAPI static Renderer *GetActiveRenderer();
		RNAPI static bool IsHeadless();

		RNAPI ~Renderer();

		RNAPI virtual Window *CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor = Window::SwapChainDescriptor()) = 0;
		RNAPI virtual Window *GetMainWindow() = 0;
		RNAPI virtual void SetMainWindow(Window *window) = 0;

		RNAPI void Activate();
		RNAPI virtual void Deactivate();

		RNAPI virtual void Render(Function &&function) = 0;
		RNAPI virtual void SubmitCamera(Camera *camera, Function &&function) = 0;
		RNAPI virtual void SubmitRenderPass(RenderPass *renderPass, RenderPass *previousRenderPass) = 0;

		RNAPI virtual bool SupportsTextureFormat(const String *format) const = 0;
		RNAPI virtual bool SupportsDrawMode(DrawMode mode) const = 0;

		RNAPI virtual size_t GetAlignmentForType(PrimitiveType type) const = 0;
		RNAPI virtual size_t GetSizeForType(PrimitiveType type) const = 0;

		RNAPI virtual GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) = 0;
		RNAPI virtual GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) = 0;

		RNAPI virtual ShaderLibrary *CreateShaderLibraryWithFile(const String *file) = 0;
		RNAPI virtual ShaderLibrary *CreateShaderLibraryWithSource(const String *source) = 0;

		RNAPI virtual Shader *GetDefaultShader(Shader::Type type, Shader::Options *options, Shader::UsageHint shader = Shader::UsageHint::Default) = 0;
		RNAPI virtual ShaderLibrary *GetDefaultShaderLibrary() = 0;

		RNAPI virtual Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) = 0;

		RNAPI virtual Framebuffer *CreateFramebuffer(const Vector2 &size) = 0;

		RNAPI virtual Drawable *CreateDrawable() = 0;
		RNAPI virtual void DeleteDrawable(Drawable *drawable) = 0;
		RNAPI virtual void SubmitDrawable(Drawable *drawable) = 0;

		RNAPI virtual void SubmitLight(const Light *light) = 0;

		RendererDescriptor *GetDescriptor() const { return _descriptor; }
		RenderingDevice *GetDevice() const { return _device; }

	protected:
		RNAPI Renderer(RendererDescriptor *descriptor, RenderingDevice *device);

	private:
		RenderingDevice *_device;
		RendererDescriptor *_descriptor;

		__RNDeclareMetaInternal(Renderer)
	};

	RNExceptionType(ShaderCompilation)
}


#endif /* __RAYNE_RENDERER_H_ */
