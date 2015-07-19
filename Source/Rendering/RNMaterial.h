//
//  RNMaterial.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_MATERIAL_H_
#define __RAYNE_MATERIAL_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "RNShader.h"

namespace RN
{
	class Material : public Object
	{
	public:
		RNAPI Material(Shader *fragmentShader, Shader *vertexShader);
		RNAPI ~Material() override;

		Shader *GetFragmentShader() const { return _fragmentShader; }
		Shader *GetVertexShader() const { return _vertexShader; }

	private:
		Shader *_fragmentShader;
		Shader *_vertexShader;

		RNDeclareMeta(Material)
	};
}


#endif /* __RAYNE_MATERIAL_H_ */
