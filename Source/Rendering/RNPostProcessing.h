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
#include "../Math/RNRect.h"
#include "RNRenderPass.h"
#include "RNMaterial.h"


namespace RN
{
	class PostProcessingAPIStage : public RenderPass
	{
	public:
		enum Type
		{
			ResolveMSAA,
			Blit,
			Convert
		};

		RNAPI PostProcessingAPIStage(Type type);
		RNAPI ~PostProcessingAPIStage();

		Type GetType() const { return _type; }

	private:
		Type _type;

		__RNDeclareMetaInternal(PostProcessingAPIStage)
	};

	class PostProcessingStage : public RenderPass
	{
	public:
		RNAPI PostProcessingStage();
		RNAPI ~PostProcessingStage();

		RNAPI void SetMaterial(Material *material);
		Material *GetMaterial() const { return _material; }

	private:
		Material *_material;

		__RNDeclareMetaInternal(PostProcessingStage)
	};
}


#endif /* __RAYNE_POSTPROCESSING_H__ */
