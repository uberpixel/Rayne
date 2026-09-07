//
//  RNLightClusterRendererAttachment.cpp
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLightClusterRendererAttachment.h"
#include "../Rendering/RNRenderFrame.h"
#include "RNLightClusterPassSnapshot.h"

namespace RN
{
	RNDefineMeta(LightClusterPassSnapshot, Object)
	RNDefineMeta(LightClusterRendererAttachment, RendererAttachment)

	void LightClusterRendererAttachment::RegisterShaderSources(Renderer *renderer)
	{
		RN_ASSERT(renderer, "Renderer mustn't be NULL");

		renderer->RegisterShaderSource(RNCSTR("lightClusterPointLights"), Shader::ArgumentBuffer::Source::Pass);
		renderer->RegisterShaderSource(RNCSTR("lightClusterSpotLights"), Shader::ArgumentBuffer::Source::Pass);
		renderer->RegisterShaderSource(RNCSTR("lightClusterRecords"), Shader::ArgumentBuffer::Source::Pass);
		renderer->RegisterShaderSource(RNCSTR("lightClusterIndices"), Shader::ArgumentBuffer::Source::Pass);
	}

	void LightClusterRendererAttachment::PrepareRenderFrame(Renderer *, RenderFrame &frame)
	{
		for(size_t i = 0; i < frame.GetPassCount(); i += 1)
		{
			RenderFrame::Pass &pass = frame.GetPass(i);
			LightClusterPassSnapshot *snapshot = pass.GetAttachmentSnapshot<LightClusterPassSnapshot>();
			if(!snapshot)
				continue;

			const LightManager::DrawSnapshot &drawSnapshot = snapshot->PrepareDrawSnapshot();
			pass.SetPassResourceBuffer(RNCSTR("lightClusterPointLights"), drawSnapshot.GetPointLightBuffer());
			pass.SetPassResourceBuffer(RNCSTR("lightClusterSpotLights"), drawSnapshot.GetSpotLightBuffer());
			pass.SetPassResourceBuffer(RNCSTR("lightClusterRecords"), drawSnapshot.GetClusterRecordsBuffer());
			pass.SetPassResourceBuffer(RNCSTR("lightClusterIndices"), drawSnapshot.GetClusterIndexBuffer());
		}
	}
} // namespace RN
