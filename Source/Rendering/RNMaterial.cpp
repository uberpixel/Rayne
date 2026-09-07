//
//  RNMaterial.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMaterial.h"
#include "RNFramebuffer.h"

namespace RN
{
	RNDefineMeta(Material, Object)

	Material::Properties::Properties() :
		ambientColor(Color(0.5f, 0.5f, 0.5f, 1.0f)), diffuseColor(Color(1.0f, 1.0f, 1.0f, 1.0f)), specularColor(Color(1.0f, 1.0f, 1.0f, 4.0f)), emissiveColor(Color(0.0f, 0.0f, 0.0f, 0.0f)), alphaToCoverageClamp(1.0f), textureTileFactor(1.0f)
	{}

	Material::Properties::Properties(const Properties &properties)
	{
		CopyFromProperties(properties);
	}

	Material::Properties::~Properties() = default;

	void Material::Properties::CopyFromProperties(const Properties &properties)
	{
		if(this == &properties) return;

		ambientColor = properties.ambientColor;
		diffuseColor = properties.diffuseColor;
		specularColor = properties.specularColor;
		emissiveColor = properties.emissiveColor;
		alphaToCoverageClamp = properties.alphaToCoverageClamp;
		textureTileFactor = properties.textureTileFactor;

		customMatrix1 = properties.customMatrix1;
		customMatrix2 = properties.customMatrix2;

		uiClippingRect = properties.uiClippingRect;
		uiOffset = properties.uiOffset;
		uiOutlineColor = properties.uiOutlineColor;

		_customShaderUniforms = properties._customShaderUniforms;
	}

	void Material::Properties::SetCustomShaderUniform(const String *name, Value *value)
	{
		_customShaderUniforms[name->GetHash()] = value;
	}

	void Material::Properties::SetCustomShaderUniform(const String *name, Number *number)
	{
		_customShaderUniforms[name->GetHash()] = number;
	}

	Object *Material::Properties::GetCustomShaderUniform(const String *name) const
	{
		const auto result = _customShaderUniforms.find(name->GetHash());
		if(result != _customShaderUniforms.end())
		{
			return result->second.Get();
		}

		return nullptr;
	}

	Object *Material::Properties::GetCustomShaderUniform(size_t nameHash) const
	{
		const auto result = _customShaderUniforms.find(nameHash);
		if(result != _customShaderUniforms.end())
		{
			return result->second.Get();
		}

		return nullptr;
	}

	Material::PipelineProperties::PipelineProperties() :
		colorWriteMask(0xf), depthMode(DepthMode::Greater), depthWriteEnabled(true), usePolygonOffset(false), polygonOffsetFactor(-1.1f), polygonOffsetUnits(-0.1f), useAlphaToCoverage(false), cullMode(CullMode::BackFace), blendOperationRGB(BlendOperation::None), blendOperationAlpha(BlendOperation::None), blendFactorSourceRGB(BlendFactor::SourceAlpha), blendFactorDestinationRGB(BlendFactor::OneMinusSourceAlpha), blendFactorSourceAlpha(BlendFactor::One), blendFactorDestinationAlpha(BlendFactor::One)
	{}

	Material::PipelineProperties::PipelineProperties(const PipelineProperties &properties)
	{
		CopyFromPipelineProperties(properties);
	}

	Material::PipelineProperties::~PipelineProperties()
	{

	}

	void Material::PipelineProperties::CopyFromPipelineProperties(const PipelineProperties &properties)
	{
		if(this == &properties) return;

		colorWriteMask = properties.colorWriteMask;
		depthMode = properties.depthMode;
		depthWriteEnabled = properties.depthWriteEnabled;
		usePolygonOffset = properties.usePolygonOffset;
		polygonOffsetFactor = properties.polygonOffsetFactor;
		polygonOffsetUnits = properties.polygonOffsetUnits;
		useAlphaToCoverage = properties.useAlphaToCoverage;
		cullMode = properties.cullMode;
		blendOperationRGB = properties.blendOperationRGB;
		blendOperationAlpha = properties.blendOperationAlpha;
		blendFactorSourceRGB = properties.blendFactorSourceRGB;
		blendFactorDestinationRGB = properties.blendFactorDestinationRGB;
		blendFactorSourceAlpha = properties.blendFactorSourceAlpha;
		blendFactorDestinationAlpha = properties.blendFactorDestinationAlpha;
	}

	void Material::DrawSnapshot::Reset()
	{
		_textures = nullptr;

		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			_vertexShader[i] = nullptr;
			_fragmentShader[i] = nullptr;
		}

		_override = 0;

		Properties defaultProperties;
		_properties.CopyFromProperties(defaultProperties);

		PipelineProperties defaultPipelineProperties;
		_pipelineProperties.CopyFromPipelineProperties(defaultPipelineProperties);
	}

	Shader *Material::DrawSnapshot::GetFragmentShader(Shader::UsageHint type) const
	{
		uint8 index = static_cast<uint8>(type);
		if(!_fragmentShader[index].Get() && !_vertexShader[index].Get())
			return _fragmentShader[static_cast<uint8>(Shader::UsageHint::Default)].Get();

		return _fragmentShader[index].Get();
	}

	Shader *Material::DrawSnapshot::GetVertexShader(Shader::UsageHint type) const
	{
		uint8 index = static_cast<uint8>(type);
		if(!_vertexShader[index].Get())
			return _vertexShader[static_cast<uint8>(Shader::UsageHint::Default)].Get();

		return _vertexShader[index].Get();
	}

	Shader *Material::DrawSnapshot::GetSelectedFragmentShader(Shader::UsageHint type, const DrawSnapshot *overrideMaterial) const
	{
		if(overrideMaterial && !(overrideMaterial->_override & Override::GroupShaders) && !(_override & Override::GroupShaders))
			return overrideMaterial->GetFragmentShader(type);

		return GetFragmentShader(type);
	}

	Shader *Material::DrawSnapshot::GetSelectedVertexShader(Shader::UsageHint type, const DrawSnapshot *overrideMaterial) const
	{
		if(overrideMaterial && !(overrideMaterial->_override & Override::GroupShaders) && !(_override & Override::GroupShaders))
			return overrideMaterial->GetVertexShader(type);

		return GetVertexShader(type);
	}

	void Material::DrawSnapshot::SetTextures(const Array *textures)
	{
		Array *copy = SafeCopy(textures);
		_textures = copy;
		SafeRelease(copy);
	}

	void Material::DrawSnapshot::GetMergedProperties(const DrawSnapshot *overrideMaterial, Properties &properties) const
	{
		Material::MergeProperties(_properties, _override, overrideMaterial ? &overrideMaterial->_properties : nullptr, overrideMaterial ? overrideMaterial->_override : 0, properties);
	}

	void Material::DrawSnapshot::GetMergedPipelineProperties(const DrawSnapshot *overrideMaterial, PipelineProperties &properties) const
	{
		Material::MergePipelineProperties(_pipelineProperties, _override, overrideMaterial ? &overrideMaterial->_pipelineProperties : nullptr, overrideMaterial ? overrideMaterial->_override : 0, properties);
	}

	Material::Material(Shader *vertexShader, Shader *fragmentShader) :
		_override(0),
		_textures(new Array()),
		_drawSnapshotVersion(1),
		_pipelineVersion(1)
	{
		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			if(i == 0)
			{
				_vertexShader[i] = SafeRetain(vertexShader);
				_fragmentShader[i] = SafeRetain(fragmentShader);
			}
			else
			{
				_vertexShader[i] = nullptr;
				_fragmentShader[i] = nullptr;
			}

			RN_ASSERT(!_vertexShader[i] || _vertexShader[i]->GetType() == Shader::Type::Vertex, "Vertex shader must be a vertex shader");
			RN_ASSERT(!_fragmentShader[i] || _fragmentShader[i]->GetType() == Shader::Type::Fragment, "Fragment shader must be a fragment shader");
		}
	}

	Material::Material(const Material *other) :
		_override(other->_override),
		_textures(SafeCopy(other->_textures)),
		_drawSnapshotVersion(1),
		_pipelineVersion(1),
		_properties(other->_properties),
		_pipelineProperties(other->_pipelineProperties)
	{
		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			_vertexShader[i] = SafeRetain(other->_vertexShader[i]);
			_fragmentShader[i] = SafeRetain(other->_fragmentShader[i]);
		}
	}

	Material::~Material()
	{
		SafeRelease(_textures);

		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			SafeRelease(_fragmentShader[i]);
			SafeRelease(_vertexShader[i]);
		}
	}

	Material *Material::WithShaders(Shader *vertexShader, Shader *fragmentShader)
	{
		Material *material = new Material(vertexShader, fragmentShader);
		return material->Autorelease();
	}

	Material *Material::WithMaterial(const Material *material)
	{
		Material *copyMaterial = new Material(material);
		return copyMaterial->Autorelease();
	}

	void Material::SetTextures(const Array *textures)
	{
		if(_textures->IsEqualLite(textures)) return;
		SafeRelease(_textures);
		_textures = textures->Copy();
		MarkDrawSnapshotDirty();
	}

	void Material::AddTexture(Framebuffer *texture)
	{
		_textures->AddObject(texture);
		MarkDrawSnapshotDirty();
	}

	void Material::AddTexture(Texture *texture)
	{
		_textures->AddObject(texture);
		MarkDrawSnapshotDirty();
	}

	void Material::RemoveAllTextures()
	{
		if(_textures->GetCount() == 0) return;
		_textures->RemoveAllObjects();
		MarkDrawSnapshotDirty();
	}

	void Material::SetFragmentShader(Shader *shader, Shader::UsageHint type)
	{
		RN_ASSERT(shader, "A valid fragment shader needs to be assigned!");
		if(_fragmentShader[type] == shader) return;
		SafeRelease(_fragmentShader[type]);
		_fragmentShader[type] = SafeRetain(shader);
		RN_ASSERT(!_fragmentShader[type] || _fragmentShader[type]->GetType() == Shader::Type::Fragment, "Fragment shader must be a fragment shader");
		MarkPipelineDirty();
	}

	void Material::SetVertexShader(Shader *shader, Shader::UsageHint type)
	{
		RN_ASSERT(shader, "A valid vertex shader needs to be assigned!");
		if(_vertexShader[type] == shader) return;
		SafeRelease(_vertexShader[type]);
		_vertexShader[type] = SafeRetain(shader);
		RN_ASSERT(!_vertexShader[type] || _vertexShader[type]->GetType() == Shader::Type::Vertex, "Vertex shader must be a vertex shader");
		MarkPipelineDirty();
	}

	void Material::SetOverride(Override override)
	{
		if(_override == override) return;
		_override = override;
		MarkPipelineDirty();
	}

	void Material::SetColorWriteMask(bool writeRed, bool writeGreen, bool writeBlue, bool writeAlpha)
	{
		uint8 mask = (writeRed ? 1 : 0) | (writeGreen ? 2 : 0) | (writeBlue ? 4 : 0) | (writeAlpha ? 8 : 0);
		if(_pipelineProperties.colorWriteMask == mask) return;
		_pipelineProperties.colorWriteMask = mask;
		MarkPipelineDirty();
	}

	void Material::SetDepthWriteEnabled(bool depthWrite)
	{
		if(_pipelineProperties.depthWriteEnabled == depthWrite) return;
		_pipelineProperties.depthWriteEnabled = depthWrite;
		MarkPipelineDirty();
	}
	void Material::SetDepthMode(DepthMode mode)
	{
		if(_pipelineProperties.depthMode == mode) return;
		_pipelineProperties.depthMode = mode;
		MarkPipelineDirty();
	}

	void Material::SetTextureTileFactor(const Vector2 &factor)
	{
		if(_properties.textureTileFactor.x == factor.x && _properties.textureTileFactor.y == factor.y) return;
		_properties.textureTileFactor = factor;
		MarkDrawSnapshotDirty();
	}

	void Material::SetAmbientColor(const Color &color)
	{
		if(_properties.ambientColor.r == color.r && _properties.ambientColor.g == color.g && _properties.ambientColor.b == color.b && _properties.ambientColor.a == color.a) return;
		_properties.ambientColor = color;
		MarkDrawSnapshotDirty();
	}
	void Material::SetDiffuseColor(const Color &color)
	{
		if(_properties.diffuseColor.r == color.r && _properties.diffuseColor.g == color.g && _properties.diffuseColor.b == color.b && _properties.diffuseColor.a == color.a) return;
		_properties.diffuseColor = color;
		MarkDrawSnapshotDirty();
	}
	void Material::SetSpecularColor(const Color &color)
	{
		if(_properties.specularColor.r == color.r && _properties.specularColor.g == color.g && _properties.specularColor.b == color.b && _properties.specularColor.a == color.a) return;
		_properties.specularColor = color;
		MarkDrawSnapshotDirty();
	}
	void Material::SetEmissiveColor(const Color &color)
	{
		if(_properties.emissiveColor.r == color.r && _properties.emissiveColor.g == color.g && _properties.emissiveColor.b == color.b && _properties.emissiveColor.a == color.a) return;
		_properties.emissiveColor = color;
		MarkDrawSnapshotDirty();
	}

	void Material::SetCustomMatrix1(const Matrix &matrix)
	{
		if(memcmp(_properties.customMatrix1.m, matrix.m, sizeof(matrix.m)) == 0) return;
		_properties.customMatrix1 = matrix;
		MarkDrawSnapshotDirty();
	}
	void Material::SetCustomMatrix2(const Matrix &matrix)
	{
		if(memcmp(_properties.customMatrix2.m, matrix.m, sizeof(matrix.m)) == 0) return;
		_properties.customMatrix2 = matrix;
		MarkDrawSnapshotDirty();
	}

	void Material::SetCullMode(CullMode mode)
	{
		if(_pipelineProperties.cullMode == mode) return;
		_pipelineProperties.cullMode = mode;
		MarkPipelineDirty();
	}

	void Material::SetPolygonOffset(bool enable, float factor, float units)
	{
		if(_pipelineProperties.usePolygonOffset == enable && _pipelineProperties.polygonOffsetFactor == -factor && _pipelineProperties.polygonOffsetUnits == -units) return;
		_pipelineProperties.usePolygonOffset = enable;
		_pipelineProperties.polygonOffsetFactor = -factor;
		_pipelineProperties.polygonOffsetUnits = -units;
		MarkPipelineDirty();
	}

	void Material::SetAlphaToCoverage(bool enabled, float min, float max)
	{
		if(_pipelineProperties.useAlphaToCoverage == enabled && _properties.alphaToCoverageClamp.x == min && _properties.alphaToCoverageClamp.y == max) return;
		_pipelineProperties.useAlphaToCoverage = enabled;
		_properties.alphaToCoverageClamp.x = min;
		_properties.alphaToCoverageClamp.y = max;
		MarkPipelineDirty();
	}

	void Material::SetUIClippingRect(Vector4 rect)
	{
		if(_properties.uiClippingRect.x == rect.x && _properties.uiClippingRect.y == rect.y && _properties.uiClippingRect.z == rect.z && _properties.uiClippingRect.w == rect.w) return;
		_properties.uiClippingRect = rect;
		MarkDrawSnapshotDirty();
	}

	void Material::SetUIOffset(Vector2 offset)
	{
		if(_properties.uiOffset.x == offset.x && _properties.uiOffset.y == offset.y) return;
		_properties.uiOffset = offset;
		MarkDrawSnapshotDirty();
	}

	void Material::SetUIOutlineColor(Color color)
	{
		if(_properties.uiOutlineColor.r == color.r && _properties.uiOutlineColor.g == color.g && _properties.uiOutlineColor.b == color.b && _properties.uiOutlineColor.a == color.a) return;
		_properties.uiOutlineColor = color;
		MarkDrawSnapshotDirty();
	}

	void Material::SetBlendOperation(BlendOperation blendOperationRGB, BlendOperation blendOperationAlpha)
	{
		RN_ASSERT((blendOperationRGB != BlendOperation::None && blendOperationAlpha != BlendOperation::None) || blendOperationAlpha == blendOperationRGB, "Blend operation None can not be mixed with any of the others.");
		if(_pipelineProperties.blendOperationRGB == blendOperationRGB && _pipelineProperties.blendOperationAlpha == blendOperationAlpha) return;
		_pipelineProperties.blendOperationRGB = blendOperationRGB;
		_pipelineProperties.blendOperationAlpha = blendOperationAlpha;
		MarkPipelineDirty();
	}

	void Material::SetBlendFactorSource(BlendFactor blendFactorRGB, BlendFactor blendFactorAlpha)
	{
		if(_pipelineProperties.blendFactorSourceRGB == blendFactorRGB && _pipelineProperties.blendFactorSourceAlpha == blendFactorAlpha) return;
		_pipelineProperties.blendFactorSourceRGB = blendFactorRGB;
		_pipelineProperties.blendFactorSourceAlpha = blendFactorAlpha;
		MarkPipelineDirty();
	}

	void Material::SetBlendFactorDestination(BlendFactor blendFactorRGB, BlendFactor blendFactorAlpha)
	{
		if(_pipelineProperties.blendFactorDestinationRGB == blendFactorRGB && _pipelineProperties.blendFactorDestinationAlpha == blendFactorAlpha) return;
		_pipelineProperties.blendFactorDestinationRGB = blendFactorRGB;
		_pipelineProperties.blendFactorDestinationAlpha = blendFactorAlpha;
		MarkPipelineDirty();
	}

	void Material::SetCustomShaderUniform(const String *name, Value *value)
	{
		_properties.SetCustomShaderUniform(name, value);
		MarkDrawSnapshotDirty();
	}

	void Material::SetCustomShaderUniform(const String *name, Number *number)
	{
		_properties.SetCustomShaderUniform(name, number);
		MarkDrawSnapshotDirty();
	}

	Object *Material::GetCustomShaderUniform(const String *name) const
	{
		return _properties.GetCustomShaderUniform(name);
	}

	Shader *Material::GetFragmentShader(Shader::UsageHint type) const
	{
		if(!_fragmentShader[static_cast<uint8>(type)] && !_vertexShader[static_cast<uint8>(type)])
			return _fragmentShader[static_cast<uint8>(Shader::UsageHint::Default)];

		return _fragmentShader[static_cast<uint8>(type)];
	}
	Shader *Material::GetVertexShader(Shader::UsageHint type) const
	{
		if(!_vertexShader[static_cast<uint8>(type)])
			return _vertexShader[static_cast<uint8>(Shader::UsageHint::Default)];

		return _vertexShader[static_cast<uint8>(type)];
	}

	void Material::MergeProperties(const Properties &properties, uint32 override, const Properties *overrideProperties, uint32 overrideMaterialOverride, Properties &result)
	{
		if(!overrideProperties)
		{
			result.CopyFromProperties(properties);
			return;
		}

		if(!(overrideMaterialOverride & Override::GroupColors) && !(override & Override::GroupColors))
		{
			result.ambientColor = overrideProperties->ambientColor;
			result.diffuseColor = overrideProperties->diffuseColor;
			result.specularColor = overrideProperties->specularColor;
			result.emissiveColor = overrideProperties->emissiveColor;
		}
		else
		{
			result.ambientColor = properties.ambientColor;
			result.diffuseColor = properties.diffuseColor;
			result.specularColor = properties.specularColor;
			result.emissiveColor = properties.emissiveColor;
		}

		if(!(overrideMaterialOverride & Override::GroupAlphaToCoverage) && !(override & Override::GroupAlphaToCoverage))
		{
			result.alphaToCoverageClamp = overrideProperties->alphaToCoverageClamp;
		}
		else
		{
			result.alphaToCoverageClamp = properties.alphaToCoverageClamp;
		}

		if(!(overrideMaterialOverride & Override::TextureTileFactor) && !(override & Override::TextureTileFactor))
		{
			result.textureTileFactor = overrideProperties->textureTileFactor;
		}
		else
		{
			result.textureTileFactor = properties.textureTileFactor;
		}

		result.customMatrix1 = properties.customMatrix1;
		result.customMatrix2 = properties.customMatrix2;

		result.uiClippingRect = properties.uiClippingRect;
		result.uiOffset = properties.uiOffset;
		result.uiOutlineColor = properties.uiOutlineColor;

		result._customShaderUniforms = properties._customShaderUniforms;
		if(!(overrideMaterialOverride & Override::CustomUniforms) && !(override & Override::CustomUniforms) && overrideProperties->_customShaderUniforms.size() > 0)
		{
			for(auto const &data : overrideProperties->_customShaderUniforms)
			{
				result._customShaderUniforms[data.first] = data.second;
			}
		}
	}

	void Material::MergePipelineProperties(const PipelineProperties &properties, uint32 override, const PipelineProperties *overridePipelineProperties, uint32 overrideMaterialOverride, PipelineProperties &result)
	{
		if(!overridePipelineProperties)
		{
			result.CopyFromPipelineProperties(properties);
			return;
		}

		if(!(overrideMaterialOverride & Override::ColorWriteMask) && !(override & Override::ColorWriteMask))
		{
			result.colorWriteMask = overridePipelineProperties->colorWriteMask;
		}
		else
		{
			result.colorWriteMask = properties.colorWriteMask;
		}

		if(!(overrideMaterialOverride & Override::DepthWrite) && !(override & Override::DepthWrite))
		{
			result.depthWriteEnabled = overridePipelineProperties->depthWriteEnabled;
		}
		else
		{
			result.depthWriteEnabled = properties.depthWriteEnabled;
		}

		if(!(overrideMaterialOverride & Override::GroupDepth) && !(override & Override::GroupDepth))
		{
			result.depthMode = overridePipelineProperties->depthMode;
		}
		else
		{
			result.depthMode = properties.depthMode;
		}

		if(!(overrideMaterialOverride & Override::GroupAlphaToCoverage) && !(override & Override::GroupAlphaToCoverage))
		{
			result.useAlphaToCoverage = overridePipelineProperties->useAlphaToCoverage;
		}
		else
		{
			result.useAlphaToCoverage = properties.useAlphaToCoverage;
		}

		if(!(overrideMaterialOverride & Override::GroupBlending) && !(override & Override::GroupBlending))
		{
			result.blendOperationRGB = overridePipelineProperties->blendOperationRGB;
			result.blendOperationAlpha = overridePipelineProperties->blendOperationAlpha;
			result.blendFactorSourceRGB = overridePipelineProperties->blendFactorSourceRGB;
			result.blendFactorSourceAlpha = overridePipelineProperties->blendFactorSourceAlpha;
			result.blendFactorDestinationRGB = overridePipelineProperties->blendFactorDestinationRGB;
			result.blendFactorDestinationAlpha = overridePipelineProperties->blendFactorDestinationAlpha;
		}
		else
		{
			result.blendOperationRGB = properties.blendOperationRGB;
			result.blendOperationAlpha = properties.blendOperationAlpha;
			result.blendFactorSourceRGB = properties.blendFactorSourceRGB;
			result.blendFactorSourceAlpha = properties.blendFactorSourceAlpha;
			result.blendFactorDestinationRGB = properties.blendFactorDestinationRGB;
			result.blendFactorDestinationAlpha = properties.blendFactorDestinationAlpha;
		}

		if(!(overrideMaterialOverride & Override::GroupPolygonOffset) && !(override & Override::GroupPolygonOffset))
		{
			result.usePolygonOffset = overridePipelineProperties->usePolygonOffset;
			result.polygonOffsetFactor = overridePipelineProperties->polygonOffsetFactor;
			result.polygonOffsetUnits = overridePipelineProperties->polygonOffsetUnits;
		}
		else
		{
			result.usePolygonOffset = properties.usePolygonOffset;
			result.polygonOffsetFactor = properties.polygonOffsetFactor;
			result.polygonOffsetUnits = properties.polygonOffsetUnits;
		}

		if(!(overrideMaterialOverride & Override::CullMode) && !(override & Override::CullMode))
		{
			result.cullMode = overridePipelineProperties->cullMode;
		}
		else
		{
			result.cullMode = properties.cullMode;
		}
	}

	const std::shared_ptr<const Material::DrawSnapshot> &Material::GetSharedDrawSnapshot() const
	{
		if(!_sharedDrawSnapshot)
		{
			auto snapshot = std::make_shared<DrawSnapshot>();
			GetDrawSnapshot(*snapshot);
			_sharedDrawSnapshot = std::move(snapshot);
		}
		return _sharedDrawSnapshot;
	}

	void Material::GetDrawSnapshot(DrawSnapshot &snapshot) const
	{
		snapshot._override = _override;
		snapshot.SetTextures(_textures);

		snapshot._properties.CopyFromProperties(_properties);
		snapshot._pipelineProperties.CopyFromPipelineProperties(_pipelineProperties);

		for(uint8 i = 0; i < static_cast<uint8>(Shader::UsageHint::COUNT); i++)
		{
			snapshot._vertexShader[i] = _vertexShader[i];
			snapshot._fragmentShader[i] = _fragmentShader[i];
		}
	}
} // namespace RN
