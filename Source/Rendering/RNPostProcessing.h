//
//  RNPostProcessing.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_POSTPROCESSING_H__
#define __RAYNE_POSTPROCESSING_H__

#include "../Base/RNBase.h"
#include "../Rendering/RNFramebuffer.h"
#include "../Rendering/RNMaterial.h"
#include "../Math/RNRect.h"

namespace RN
{
	class PostProcessingStage : public Object
	{
	public:
		friend class Scene;
		friend class Light;

		RNAPI PostProcessingStage();
		RNAPI ~PostProcessingStage();

		RNAPI void SetInputFramebuffer(Framebuffer *framebuffer);
		RNAPI void SetOutputFramebuffer(Framebuffer *framebuffer);
		RNAPI void SetFrame(const Rect &frame);
		RNAPI void SetMaterial(Material *material);

		Framebuffer *GetInputFramebuffer() const { return _inputFramebuffer; }
		Framebuffer *GetOutputFramebuffer() const { return _outputFramebuffer; }
		const Rect &GetFrame() const { return _frame; };
		Material *GetMaterial() const { return _material; }

	private:
		void Initialize();

		Rect _frame;
		Framebuffer *_inputFramebuffer;
		Framebuffer *_outputFramebuffer;
		Material *_material;

		__RNDeclareMetaInternal(PostProcessingStage)
	};
}


#endif /* __RAYNE_POSTPROCESSING_H__ */
