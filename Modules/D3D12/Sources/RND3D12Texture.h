//
//  RND3D12Texture.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12TEXTURE_H_
#define __RAYNE_D3D12TEXTURE_H_

#include "RND3D12.h"

namespace RN
{
	class D3D12Renderer;
	class D3D12StateCoordinator;
	class D3D12CommandList;

	class D3D12Texture : public Texture
	{
	public:
		friend class D3D12Renderer;
		friend class D3D12Framebuffer;

		D3DAPI D3D12Texture(const Descriptor &descriptor, D3D12Renderer *renderer);
		D3DAPI ~D3D12Texture() override;

		D3DAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		D3DAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		D3DAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;
		D3DAPI void GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const final;

		D3DAPI void GenerateMipMaps() final;
		D3DAPI bool HasColorChannel(ColorChannel channel) const final;

	private:
		void TransitionToState(D3D12CommandList *commandList, D3D12_RESOURCE_STATES targetState);

		D3D12Renderer *_renderer;
		D3D12StateCoordinator *_coordinator;

		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDescriptor;
		ID3D12Resource *_resource;
		D3D12_RESOURCE_STATES _currentState;

		bool _isReady;
		bool _needsMipMaps;

		RNDeclareMetaAPI(D3D12Texture, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12TEXTURE_H_ */
