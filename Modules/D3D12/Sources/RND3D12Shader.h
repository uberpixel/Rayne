//
//  RND3D12Shader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12SHADER_H_
#define __RAYNE_D3D12SHADER_H_

#include <Rayne.h>

namespace RN
{
	class D3D12ShaderLibrary;
	class D3D12Shader : public Shader
	{
	public:
		friend class D3D12ShaderLibrary;
		friend class D3D12StateCoordinator;

		class D3D12Attribute : public Attribute
		{
		public:
			D3D12Attribute(const String *name, PrimitiveType type, size_t index) :
				Attribute(name, type),
				_index(index)
			{}

			size_t GetIndex() const { return _index; }

		private:
			size_t _index;
		};

		RNAPI ~D3D12Shader() override;

		RNAPI const String *GetName() const override;
		RNAPI const Array *GetAttributes() const override;

	private:
		D3D12Shader(String *file, String *entryPointName, String *shaderType);

		Array *_attributes;

		RNDeclareMeta(D3D12Shader)
	};
}


#endif /* __RAYNE_D3D12SHADER_H_ */
