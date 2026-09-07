//
//  RNRenderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderer.h"
#include "RNRenderFrame.h"
#include "RNRenderPassResources.h"
#include "RNShadowRendererAttachment.h"
#include "../Base/RNKernel.h"
#include "../Debug/RNLogger.h"
#include "../Math/RNHalfVector.h"
#include "../Scene/RNLightClusterRendererAttachment.h"

namespace RN
{
	RNDefineMeta(RenderFramePresentationState, Object)
	RNDefineMeta(RenderPassDependencyProvider, Object)
	RNDefineMeta(RendererAttachment, Object)
	RNDefineMeta(Renderer, Object)

	RNExceptionImp(ShaderCompilation)

	static Renderer *_activeRenderer = nullptr;

	Renderer::Renderer(RendererDescriptor *descriptor, RenderingDevice *device) :
		_frameStatisticsTimer(0.0),
		_lastStartedRenderFrameID(0),
		_completedRenderFrameID(0),
		_lastRenderFrameDrawItemCount(0),
		_activeRenderFrame(nullptr),
		_hasResolvedShaderSources(false),
		_device(device),
		_descriptor(descriptor)
	{
		RN_ASSERT(descriptor, "Descriptor mustn't be NULL");
		RN_ASSERT(device, "Device mustn't be NULL");

		RegisterDefaultShaderSources();

		for(size_t typeIndex = 0; typeIndex < Shader::Type::COUNT; typeIndex++)
		{
			for(size_t hintIndex = 0; hintIndex < Shader::UsageHint::COUNT; hintIndex++)
			{
				_defaultShaderCache[typeIndex][hintIndex] = nullptr;
			}
		}
	}

	Renderer::~Renderer()
	{
		for(size_t typeIndex = 0; typeIndex < Shader::Type::COUNT; typeIndex++)
		{
			for(size_t hintIndex = 0; hintIndex < Shader::UsageHint::COUNT; hintIndex++)
			{
				SafeRelease(_defaultShaderCache[typeIndex][hintIndex]);
			}
		}

		if(_activeRenderer == this)
			_activeRenderer = nullptr;
	}

	bool Renderer::IsHeadless()
	{
		return !_activeRenderer;
	}

	Renderer *Renderer::GetActiveRenderer()
	{
		RN_ASSERT(_activeRenderer, "GetActiveRenderer() called, but no renderer is currently active");
		return _activeRenderer;
	}

	RenderFramePresentationState::RenderFramePresentationState()
	{}

	RenderFramePresentationState::~RenderFramePresentationState()
	{}

	RenderPassDependencyProvider::RenderPassDependencyProvider()
	{}

	RenderPassDependencyProvider::~RenderPassDependencyProvider()
	{}

	void RenderPassDependencyProvider::CollectRenderPassDependencies(const RenderFrame::Pass &, RenderPassDependencyCollector &) const
	{}

	RendererAttachment::RendererAttachment()
	{}

	RendererAttachment::~RendererAttachment()
	{}

	void RendererAttachment::PrepareRenderFrame(Renderer *, RenderFrame &)
	{}

	bool RenderFramePresentationState::BeginFrameOnRenderThread()
	{
		return true;
	}

	void RenderFramePresentationState::EndFrameOnRenderThread()
	{}

	void RenderFramePresentationState::CancelFrameOnRenderThread()
	{}

	Matrix Renderer::GetProjectionCorrectionMatrix() const
	{
		return Matrix();
	}

	bool Renderer::FillCommonUniform(Shader::UniformDescriptor *descriptor, uint8 *buffer, const RenderFrame::CameraSnapshot *cameraSnapshot, const std::vector<RenderFrame::CameraSnapshot> *multiviewCameraSnapshots) const
	{
		switch(descriptor->GetIdentifier())
		{
			case Shader::UniformDescriptor::Identifier::Time:
			{
				float time = static_cast<float>(Kernel::GetSharedInstance()->GetTotalTime());
				std::memcpy(buffer + descriptor->GetOffset(), &time, std::min(descriptor->GetSize(), sizeof(time)));
				return true;
			}

			default:
				break;
		}

		const bool hasCameraSnapshot = cameraSnapshot && cameraSnapshot->GetSourceCameraUID() != RenderFrame::CameraSnapshot::InvalidSourceCameraUID;
		const bool hasMultiviewSnapshots = multiviewCameraSnapshots && !multiviewCameraSnapshots->empty();
		if(!hasCameraSnapshot && !hasMultiviewSnapshots)
			return false;

		switch(descriptor->GetIdentifier())
		{
			case Shader::UniformDescriptor::Identifier::ViewMatrix:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), cameraSnapshot->GetViewMatrix().m, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::ViewMatrixMultiview:
			{
				if(!hasMultiviewSnapshots) return true;
				size_t viewCount = std::min(multiviewCameraSnapshots->size(), descriptor->GetElementCount());
				for(size_t i = 0; i < viewCount; i += 1)
				{
					std::memcpy(buffer + descriptor->GetOffset() + 64 * i, (*multiviewCameraSnapshots)[i].GetViewMatrix().m, 64);
				}
				return true;
			}

			case Shader::UniformDescriptor::Identifier::ViewProjectionMatrix:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), cameraSnapshot->GetProjectionViewMatrix().m, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::ViewProjectionMatrixMultiview:
			{
				if(!hasMultiviewSnapshots) return true;
				size_t viewCount = std::min(multiviewCameraSnapshots->size(), descriptor->GetElementCount());
				for(size_t i = 0; i < viewCount; i += 1)
				{
					std::memcpy(buffer + descriptor->GetOffset() + 64 * i, (*multiviewCameraSnapshots)[i].GetProjectionViewMatrix().m, 64);
				}
				return true;
			}

			case Shader::UniformDescriptor::Identifier::ProjectionMatrix:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), cameraSnapshot->GetProjectionMatrix().m, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::ProjectionMatrixMultiview:
			{
				if(!hasMultiviewSnapshots) return true;
				size_t viewCount = std::min(multiviewCameraSnapshots->size(), descriptor->GetElementCount());
				for(size_t i = 0; i < viewCount; i += 1)
				{
					std::memcpy(buffer + descriptor->GetOffset() + 64 * i, (*multiviewCameraSnapshots)[i].GetProjectionMatrix().m, 64);
				}
				return true;
			}

			case Shader::UniformDescriptor::Identifier::InverseViewMatrix:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), cameraSnapshot->GetInverseViewMatrix().m, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::InverseViewMatrixMultiview:
			{
				if(!hasMultiviewSnapshots) return true;
				size_t viewCount = std::min(multiviewCameraSnapshots->size(), descriptor->GetElementCount());
				for(size_t i = 0; i < viewCount; i += 1)
				{
					std::memcpy(buffer + descriptor->GetOffset() + 64 * i, (*multiviewCameraSnapshots)[i].GetInverseViewMatrix().m, 64);
				}
				return true;
			}

			case Shader::UniformDescriptor::Identifier::InverseViewProjectionMatrix:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), cameraSnapshot->GetInverseProjectionViewMatrix().m, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::InverseViewProjectionMatrixMultiview:
			{
				if(!hasMultiviewSnapshots) return true;
				size_t viewCount = std::min(multiviewCameraSnapshots->size(), descriptor->GetElementCount());
				for(size_t i = 0; i < viewCount; i += 1)
				{
					std::memcpy(buffer + descriptor->GetOffset() + 64 * i, (*multiviewCameraSnapshots)[i].GetInverseProjectionViewMatrix().m, 64);
				}
				return true;
			}

			case Shader::UniformDescriptor::Identifier::InverseProjectionMatrix:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), cameraSnapshot->GetInverseProjectionMatrix().m, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::InverseProjectionMatrixMultiview:
			{
				if(!hasMultiviewSnapshots) return true;
				size_t viewCount = std::min(multiviewCameraSnapshots->size(), descriptor->GetElementCount());
				for(size_t i = 0; i < viewCount; i += 1)
				{
					std::memcpy(buffer + descriptor->GetOffset() + 64 * i, (*multiviewCameraSnapshots)[i].GetInverseProjectionMatrix().m, 64);
				}
				return true;
			}

			case Shader::UniformDescriptor::Identifier::CameraPosition:
			{
				if(!hasCameraSnapshot) return false;
				Vector4 cameraPosition(cameraSnapshot->GetViewPosition(), 0.0f);
				std::memcpy(buffer + descriptor->GetOffset(), &cameraPosition.x, descriptor->GetSize());
				return true;
			}

			case Shader::UniformDescriptor::Identifier::CameraPositionMultiview:
			{
				if(!hasMultiviewSnapshots) return true;
				size_t viewCount = std::min(multiviewCameraSnapshots->size(), descriptor->GetElementCount());
				for(size_t i = 0; i < viewCount; i += 1)
				{
					Vector4 cameraPosition((*multiviewCameraSnapshots)[i].GetViewPosition(), 0.0f);
					std::memcpy(buffer + descriptor->GetOffset() + 16 * i, &cameraPosition.x, 16);
				}
				return true;
			}

			case Shader::UniformDescriptor::Identifier::CameraFrustumPlanes:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), cameraSnapshot->GetFrustumPlanes(), std::min(descriptor->GetSize(), sizeof(Vector4) * RenderFrame::CameraSnapshot::FrustumPlaneCount));
				return true;

			case Shader::UniformDescriptor::Identifier::CameraFrustumPlanesMultiview:
			{
				if(!hasMultiviewSnapshots) return true;
				size_t viewCount = std::min(multiviewCameraSnapshots->size(), descriptor->GetElementCount() / RenderFrame::CameraSnapshot::FrustumPlaneCount);
				for(size_t i = 0; i < viewCount; i += 1)
				{
					std::memcpy(buffer + descriptor->GetOffset() + sizeof(Vector4) * RenderFrame::CameraSnapshot::FrustumPlaneCount * i, (*multiviewCameraSnapshots)[i].GetFrustumPlanes(), sizeof(Vector4) * RenderFrame::CameraSnapshot::FrustumPlaneCount);
				}
				return true;
			}

			case Shader::UniformDescriptor::Identifier::CameraClipDistance:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), &cameraSnapshot->GetClipDistance().x, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::CameraFogDistance:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), &cameraSnapshot->GetFogDistance().x, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::CameraTag:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), &cameraSnapshot->GetTag(), descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::CameraViewport:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), &cameraSnapshot->GetFrame().x, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::CameraAmbientColor:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), &cameraSnapshot->GetAmbientColor().r, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::CameraCustomData:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), &cameraSnapshot->GetCustomData().x, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::CameraFogColor0:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), &cameraSnapshot->GetFogColor0().r, descriptor->GetSize());
				return true;

			case Shader::UniformDescriptor::Identifier::CameraFogColor1:
				if(!hasCameraSnapshot) return false;
				std::memcpy(buffer + descriptor->GetOffset(), &cameraSnapshot->GetFogColor1().r, descriptor->GetSize());
				return true;

			default:
				return false;
		}
	}

	void Renderer::FillDrawUniformBuffer(Shader::ArgumentBuffer *argumentBuffer, uint8 *buffer, const RenderFrame::DrawItem &drawItem, const Material::Properties &materialProperties, const RenderFrame::Pass &framePass) const
	{
		const RenderFrame::CameraSnapshot &cameraSnapshot = framePass.GetCameraSnapshot();
		const std::vector<RenderFrame::CameraSnapshot> &multiviewCameraSnapshots = framePass.GetMultiviewCameraSnapshots();
		const Matrix &modelMatrix = drawItem.GetModelMatrix();
		const Matrix &inverseModelMatrix = drawItem.GetInverseModelMatrix();

		const Array *uniformDescriptors = argumentBuffer->GetUniformDescriptors();
		size_t count = uniformDescriptors->GetCount();
		for(size_t index = 0; index < count; index += 1)
		{
			Shader::UniformDescriptor *descriptor = static_cast<Shader::UniformDescriptor *>(uniformDescriptors->GetObjectAtIndex(index));
			if(descriptor->GetSource() == Shader::UniformDescriptor::Source::Pass)
			{
				framePass.CopyPassUniform(descriptor->GetNameHash(), buffer + descriptor->GetOffset(), descriptor->GetSize());
				continue;
			}
			if(FillCommonUniform(descriptor, buffer, &cameraSnapshot, &multiviewCameraSnapshots))
				continue;

			switch(descriptor->GetIdentifier())
			{
				case Shader::UniformDescriptor::Identifier::ModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), modelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), inverseModelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::NormalMatrix:
				{
					Matrix normalMatrix = inverseModelMatrix.GetTransposed();
					std::memcpy(buffer + descriptor->GetOffset(), &normalMatrix.m[0], 12);
					std::memcpy(buffer + descriptor->GetOffset() + 16, &normalMatrix.m[4], 12);
					std::memcpy(buffer + descriptor->GetOffset() + 32, &normalMatrix.m[8], 12);
					break;
				}

				case Shader::UniformDescriptor::Identifier::SceneNodeUID:
				{
					uint32 sceneNodeUID = drawItem.GetSourceNodeUID() == RenderFrame::InvalidSourceNodeUID ? 0 : static_cast<uint32>(drawItem.GetSourceNodeUID());
					std::memset(buffer + descriptor->GetOffset(), 0, descriptor->GetSize());
					std::memcpy(buffer + descriptor->GetOffset(), &sceneNodeUID, std::min(descriptor->GetSize(), sizeof(sceneNodeUID)));
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewMatrix:
				{
					Matrix result = cameraSnapshot.GetViewMatrix() * modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewMatrixMultiview:
				{
					if(multiviewCameraSnapshots.size() > 0)
					{
						size_t viewCount = std::min(multiviewCameraSnapshots.size(), descriptor->GetElementCount());
						for(size_t i = 0; i < viewCount; i += 1)
						{
							Matrix result = multiviewCameraSnapshots[i].GetViewMatrix() * modelMatrix;
							std::memcpy(buffer + descriptor->GetOffset() + 64 * i, result.m, 64);
						}
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewProjectionMatrix:
				{
					Matrix result = cameraSnapshot.GetProjectionViewMatrix() * modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewProjectionMatrixMultiview:
				{
					if(multiviewCameraSnapshots.size() > 0)
					{
						size_t viewCount = std::min(multiviewCameraSnapshots.size(), descriptor->GetElementCount());
						for(size_t i = 0; i < viewCount; i += 1)
						{
							Matrix result = multiviewCameraSnapshots[i].GetProjectionViewMatrix() * modelMatrix;
							std::memcpy(buffer + descriptor->GetOffset() + 64 * i, result.m, 64);
						}
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewMatrix:
				{
					Matrix result = inverseModelMatrix * cameraSnapshot.GetInverseViewMatrix();
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewMatrixMultiview:
				{
					if(multiviewCameraSnapshots.size() > 0)
					{
						size_t viewCount = std::min(multiviewCameraSnapshots.size(), descriptor->GetElementCount());
						for(size_t i = 0; i < viewCount; i += 1)
						{
							Matrix result = inverseModelMatrix * multiviewCameraSnapshots[i].GetInverseViewMatrix();
							std::memcpy(buffer + descriptor->GetOffset() + 64 * i, result.m, 64);
						}
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewProjectionMatrix:
				{
					Matrix result = inverseModelMatrix * cameraSnapshot.GetInverseProjectionViewMatrix();
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewProjectionMatrixMultiview:
				{
					if(multiviewCameraSnapshots.size() > 0)
					{
						size_t viewCount = std::min(multiviewCameraSnapshots.size(), descriptor->GetElementCount());
						for(size_t i = 0; i < viewCount; i += 1)
						{
							Matrix result = inverseModelMatrix * multiviewCameraSnapshots[i].GetInverseProjectionViewMatrix();
							std::memcpy(buffer + descriptor->GetOffset() + 64 * i, result.m, 64);
						}
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::AmbientColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.ambientColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DiffuseColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.diffuseColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::SpecularColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.specularColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::EmissiveColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.emissiveColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CustomMatrix1:
				{
					std::memcpy(buffer + descriptor->GetOffset(), materialProperties.customMatrix1.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CustomMatrix2:
				{
					std::memcpy(buffer + descriptor->GetOffset(), materialProperties.customMatrix2.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::UIClippingRect:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.uiClippingRect.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::UIOffset:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.uiOffset.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::UIOutlineColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.uiOutlineColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::TextureTileFactor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.textureTileFactor.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::AlphaToCoverageClamp:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.alphaToCoverageClamp.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLightsCount:
				{
					uint32 lightCount = std::min(framePass.GetDirectionalLights().size(), descriptor->GetElementCount());
					std::memcpy(buffer + descriptor->GetOffset(), &lightCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLights:
				{
					size_t lightCount = std::min(framePass.GetDirectionalLights().size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &framePass.GetDirectionalLights()[0], (16 + 16) * lightCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::PointLights:
				{
					size_t lightCount = std::min(framePass.GetPointLights().size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &framePass.GetPointLights()[0], (12 + 4 + 16) * lightCount);
					}
					if(lightCount < descriptor->GetElementCount())
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 16) * lightCount, 0, (12 + 4 + 16) * (descriptor->GetElementCount() - lightCount));
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::SpotLights:
				{
					size_t lightCount = std::min(framePass.GetSpotLights().size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &framePass.GetSpotLights()[0], (12 + 4 + 12 + 4 + 16) * lightCount);
					}
					if(lightCount < descriptor->GetElementCount())
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 12 + 4 + 16) * lightCount, 0, (12 + 4 + 12 + 4 + 16) * (descriptor->GetElementCount() - lightCount));
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::BoneMatrices:
				{
					const std::vector<Matrix> &boneMatrices = drawItem.GetSkeleton().GetMatrices();
					if(boneMatrices.size() > 0)
					{
						size_t matrixCount = std::min(boneMatrices.size(), descriptor->GetElementCount());
						if(matrixCount > 0)
						{
							std::memcpy(buffer + descriptor->GetOffset(), &boneMatrices[0].m[0], 64 * matrixCount);
						}
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::Custom:
				{
					Object *object = materialProperties.GetCustomShaderUniform(descriptor->GetNameHash());
					if(object)
					{
						if(object->IsKindOfClass(Value::GetMetaClass()))
						{
							Value *value = object->Downcast<Value>();
							switch(value->GetValueType())
							{
								case TypeTranslator<Vector2>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector2))
									{
										Vector2 vector = value->GetValue<Vector2>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Vector3>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector3))
									{
										Vector3 vector = value->GetValue<Vector3>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Vector4>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector4))
									{
										Vector4 vector = value->GetValue<Vector4>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<HalfVector2>::value:
								{
									if(descriptor->GetType() == PrimitiveType::HalfVector2 && descriptor->GetSize() == sizeof(HalfVector2))
									{
										HalfVector2 vector = value->GetValue<HalfVector2>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<HalfVector3>::value:
								{
									if(descriptor->GetType() == PrimitiveType::HalfVector3 && descriptor->GetSize() == sizeof(HalfVector3))
									{
										HalfVector3 vector = value->GetValue<HalfVector3>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<HalfVector4>::value:
								{
									if(descriptor->GetType() == PrimitiveType::HalfVector4 && descriptor->GetSize() == sizeof(HalfVector4))
									{
										HalfVector4 vector = value->GetValue<HalfVector4>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Matrix>::value:
								{
									if(descriptor->GetSize() == sizeof(Matrix))
									{
										Matrix matrix = value->GetValue<Matrix>();
										std::memcpy(buffer + descriptor->GetOffset(), &matrix.m[0], descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Quaternion>::value:
								{
									if(descriptor->GetSize() == sizeof(Quaternion))
									{
										Quaternion quaternion = value->GetValue<Quaternion>();
										std::memcpy(buffer + descriptor->GetOffset(), &quaternion.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Color>::value:
								{
									if(descriptor->GetSize() == sizeof(Color))
									{
										Color color = value->GetValue<Color>();
										std::memcpy(buffer + descriptor->GetOffset(), &color.r, descriptor->GetSize());
									}
									break;
								}
								default:
									break;
							}
						}
						else
						{
							Number *number = object->Downcast<Number>();
							switch(number->GetType())
							{
								case Number::Type::Int8:
								{
									if(descriptor->GetSize() == sizeof(int8))
									{
										int8 value = number->GetInt8Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Int16:
								{
									if(descriptor->GetSize() == sizeof(int16))
									{
										int16 value = number->GetInt16Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Int32:
								{
									if(descriptor->GetSize() == sizeof(int32))
									{
										int32 value = number->GetInt32Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint8:
								{
									if(descriptor->GetSize() == sizeof(uint8))
									{
										uint8 value = number->GetUint8Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint16:
								{
									if(descriptor->GetSize() == sizeof(uint16))
									{
										uint16 value = number->GetUint16Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint32:
								{
									if(descriptor->GetSize() == sizeof(uint32))
									{
										uint32 value = number->GetUint32Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Float32:
								{
									if(descriptor->GetSize() == sizeof(float))
									{
										float value = number->GetFloatValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Float16:
								{
									if(descriptor->GetType() == PrimitiveType::Half && descriptor->GetSize() == sizeof(uint16))
									{
										uint16 value = number->GetHalfValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Boolean:
								{
									if(descriptor->GetSize() == sizeof(bool))
									{
										bool value = number->GetBoolValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								default:
									break;
							}
						}
					}
					break;
				}

				default:
					break;
			}
		}
	}

	void Renderer::Activate()
	{
		RN_ASSERT(!_activeRenderer, "Rayne only supports one active renderer at a time");
		_activeRenderer = this;
		Retain();
	}

	void Renderer::Deactivate()
	{
		RN_ASSERT(_activeRenderer == this, "Deactivate() must be called on the active renderer");

		Renderer *renderer = _activeRenderer;
		// Base renderer state can retain backend resources; release it while the concrete renderer is still active.
		renderer->ReleaseRendererReferences();
		renderer->Release();
		if(_activeRenderer == renderer)
			_activeRenderer = nullptr;
	}

	void Renderer::AddAttachment(RendererAttachment *attachment)
	{
		RN_ASSERT(attachment, "RendererAttachment mustn't be NULL");

		LockGuard<Lockable> lock(_rendererAttachmentsLock);
		for(const StrongRef<RendererAttachment> &existingAttachment : _rendererAttachments)
		{
			if(existingAttachment.Get() == attachment)
				return;
		}

		_rendererAttachments.push_back(attachment);
	}

	void Renderer::RemoveAttachment(RendererAttachment *attachment)
	{
		LockGuard<Lockable> lock(_rendererAttachmentsLock);
		_rendererAttachments.erase(std::remove_if(_rendererAttachments.begin(), _rendererAttachments.end(), [attachment](const StrongRef<RendererAttachment> &existingAttachment) {
			return existingAttachment.Get() == attachment;
		}), _rendererAttachments.end());
	}

	bool Renderer::HasAttachment(MetaClass *meta)
	{
		RN_ASSERT(meta, "RendererAttachment MetaClass mustn't be NULL");

		LockGuard<Lockable> lock(_rendererAttachmentsLock);
		for(const StrongRef<RendererAttachment> &attachment : _rendererAttachments)
		{
			if(attachment->IsKindOfClass(meta))
				return true;
		}

		return false;
	}

	void Renderer::SubmitAttachmentSnapshot(Object *snapshot)
	{
		RN_ASSERT(_activeRenderFrame, "SubmitAttachmentSnapshot() called outside render frame submission");
		_activeRenderFrame->AddAttachmentSnapshot(snapshot);
	}

	void Renderer::SubmitCameraPassAttachmentSnapshot(Object *snapshot)
	{
		RN_ASSERT(snapshot, "Camera pass attachment snapshot mustn't be NULL");
		RN_ASSERT(_activeRenderFrame, "SubmitCameraPassAttachmentSnapshot() called outside render frame submission");
		RN_ASSERT(!_cameraPassAttachmentSnapshotStack.empty(), "SubmitCameraPassAttachmentSnapshot() called outside camera submission");

		_cameraPassAttachmentSnapshotStack.back().push_back(snapshot);
	}

	void Renderer::RegisterShaderSource(const String *name, Shader::ArgumentBuffer::Source source)
	{
		RN_ASSERT(name, "Shader source name mustn't be NULL");
		RN_ASSERT(source == Shader::ArgumentBuffer::Source::Pass || source == Shader::ArgumentBuffer::Source::Frame, "Only pass and frame buffer shader sources can be registered");

		LockGuard<Lockable> lock(_shaderSourceRegistryLock);
		RN_ASSERT(!_hasResolvedShaderSources, "Shader sources must be registered before shader reflection");

		size_t nameHash = name->GetHash();
		auto iterator = _argumentBufferSources.find(nameHash);
		if(iterator != _argumentBufferSources.end())
		{
			RN_ASSERT(iterator->second == source, "Argument buffer source has already been registered with a different source");
			return;
		}

#if RN_BUILD_DEBUG
		TrackShaderSourceName(nameHash, name);
#endif
		_argumentBufferSources.emplace(nameHash, source);
	}

	void Renderer::RegisterShaderSource(const String *name, Shader::ArgumentTexture::Source source)
	{
		RN_ASSERT(name, "Shader source name mustn't be NULL");
		RN_ASSERT(source == Shader::ArgumentTexture::Source::Pass || source == Shader::ArgumentTexture::Source::Frame, "Only pass and frame texture shader sources can be registered");

		LockGuard<Lockable> lock(_shaderSourceRegistryLock);
		RN_ASSERT(!_hasResolvedShaderSources, "Shader sources must be registered before shader reflection");

		size_t nameHash = name->GetHash();
		auto iterator = _argumentTextureSources.find(nameHash);
		if(iterator != _argumentTextureSources.end())
		{
			RN_ASSERT(iterator->second == source, "Argument texture source has already been registered with a different source");
			return;
		}

#if RN_BUILD_DEBUG
		TrackShaderSourceName(nameHash, name);
#endif
		_argumentTextureSources.emplace(nameHash, source);
	}

	void Renderer::RegisterShaderSource(const String *name, Shader::UniformDescriptor::Source source)
	{
		RN_ASSERT(name, "Shader source name mustn't be NULL");
		RN_ASSERT(source == Shader::UniformDescriptor::Source::Pass, "Only pass uniform shader sources can be registered");

		LockGuard<Lockable> lock(_shaderSourceRegistryLock);
		RN_ASSERT(!_hasResolvedShaderSources, "Shader sources must be registered before shader reflection");

		size_t nameHash = name->GetHash();
		auto iterator = _uniformDescriptorSources.find(nameHash);
		if(iterator != _uniformDescriptorSources.end())
		{
			RN_ASSERT(iterator->second == source, "Uniform shader source has already been registered with a different source");
			return;
		}

#if RN_BUILD_DEBUG
		TrackShaderSourceName(nameHash, name);
#endif
		_uniformDescriptorSources.emplace(nameHash, source);
	}

	Shader::ArgumentBuffer::Source Renderer::GetShaderSource(const String *name, Shader::ArgumentBuffer::Source defaultSource) const
	{
		if(!name)
			return defaultSource;

		LockGuard<Lockable> lock(_shaderSourceRegistryLock);
		_hasResolvedShaderSources = true;

		size_t nameHash = name->GetHash();
		auto iterator = _argumentBufferSources.find(nameHash);
		if(iterator == _argumentBufferSources.end())
			return defaultSource;

#if RN_BUILD_DEBUG
		TrackShaderSourceName(nameHash, name);
#endif
		return iterator->second;
	}

	Shader::ArgumentTexture::Source Renderer::GetShaderSource(const String *name, Shader::ArgumentTexture::Source defaultSource) const
	{
		if(!name)
			return defaultSource;

		LockGuard<Lockable> lock(_shaderSourceRegistryLock);
		_hasResolvedShaderSources = true;

		size_t nameHash = name->GetHash();
		auto iterator = _argumentTextureSources.find(nameHash);
		if(iterator == _argumentTextureSources.end())
			return defaultSource;

#if RN_BUILD_DEBUG
		TrackShaderSourceName(nameHash, name);
#endif
		return iterator->second;
	}

	Shader::UniformDescriptor::Source Renderer::GetShaderSource(const String *name, Shader::UniformDescriptor::Source defaultSource) const
	{
		if(!name)
			return defaultSource;

		LockGuard<Lockable> lock(_shaderSourceRegistryLock);
		_hasResolvedShaderSources = true;

		size_t nameHash = name->GetHash();
		auto iterator = _uniformDescriptorSources.find(nameHash);
		if(iterator == _uniformDescriptorSources.end())
			return defaultSource;

#if RN_BUILD_DEBUG
		TrackShaderSourceName(nameHash, name);
#endif
		return iterator->second;
	}

#if RN_BUILD_DEBUG
	void Renderer::TrackShaderSourceName(size_t nameHash, const String *name) const
	{
		auto iterator = _shaderSourceNames.find(nameHash);
		RN_DEBUG_ASSERT(iterator == _shaderSourceNames.end() || iterator->second->IsEqual(name), "Shader source names have a hash collision");

		if(iterator == _shaderSourceNames.end())
			_shaderSourceNames[nameHash] = const_cast<String *>(name);
	}
#endif

	void Renderer::RegisterDefaultShaderSources()
	{
		ShadowRendererAttachment::RegisterShaderSources(this);
		LightClusterRendererAttachment::RegisterShaderSources(this);
	}

	RenderFrame *Renderer::SetActiveRenderFrame(RenderFrame *frame)
	{
		RenderFrame *previousFrame = _activeRenderFrame;
		_activeRenderFrame = frame;
		return previousFrame;
	}

	void Renderer::BeginCameraPassAttachmentSnapshots()
	{
		RN_ASSERT(_activeRenderFrame, "BeginCameraPassAttachmentSnapshots() called outside render frame submission");
		_cameraPassAttachmentSnapshotStack.emplace_back();
	}

	void Renderer::AddCameraPassAttachmentSnapshots(size_t passIndex)
	{
		RN_ASSERT(_activeRenderFrame, "AddCameraPassAttachmentSnapshots() called outside render frame submission");
		RN_ASSERT(!_cameraPassAttachmentSnapshotStack.empty(), "AddCameraPassAttachmentSnapshots() called outside camera submission");

		RenderFrame::Pass &pass = _activeRenderFrame->GetPass(passIndex);
		for(const StrongRef<Object> &snapshot : _cameraPassAttachmentSnapshotStack.back())
		{
			pass.AddAttachmentSnapshot(snapshot.Get());
		}
	}

	void Renderer::FinishCameraPassAttachmentSnapshots()
	{
		RN_ASSERT(!_cameraPassAttachmentSnapshotStack.empty(), "FinishCameraPassAttachmentSnapshots() called outside camera submission");
		_cameraPassAttachmentSnapshotStack.pop_back();
	}

	void Renderer::PrepareRendererAttachments(RenderFrame &frame)
	{
		std::vector<StrongRef<RendererAttachment>> attachments;
		{
			LockGuard<Lockable> lock(_rendererAttachmentsLock);
			attachments = _rendererAttachments;
		}

		for(const StrongRef<RendererAttachment> &attachment : attachments)
		{
			attachment->PrepareRenderFrame(this, frame);
		}
	}

	void Renderer::BeginRenderFrameSubmission(RenderFrame &frame)
	{
		size_t drawItemReserveCount;
		uint64 completedFrameID;
		{
			LockGuard<Lockable> lock(_frameLifecycleLock);
			_lastStartedRenderFrameID += 1;
			frame.SetFrameID(_lastStartedRenderFrameID);
			RN_PROFILE_ATRACE_ASYNC_BEGIN_N("RN RenderFrame", _lastStartedRenderFrameID);
			drawItemReserveCount = static_cast<size_t>(static_cast<float>(_lastRenderFrameDrawItemCount) * RN_RENDERING_DRAW_ITEM_RESERVE_MULTIPLIER);
			completedFrameID = _completedRenderFrameID;
		}

		DrainDrawSnapshots(completedFrameID);
		frame.ReserveDrawItems(drawItemReserveCount);
	}

	void Renderer::FinishRenderFrameSubmission(const RenderFrame &frame)
	{
		uint64 frameID = frame.GetFrameID();
		{
			LockGuard<Lockable> lock(_frameLifecycleLock);
			if(frameID > _completedRenderFrameID)
				_completedRenderFrameID = frameID;
			_lastRenderFrameDrawItemCount = frame.GetDrawItemCount();
		}

		RN_PROFILE_ATRACE_ASYNC_END_N("RN RenderFrame", frameID);
		FlushDeletedDrawables();
	}

	void Renderer::QueueDrawableDeletion(Drawable *drawable)
	{
		LockGuard<Lockable> lock(_frameLifecycleLock);
		_pendingDeletedDrawables.push_back({ drawable, _lastStartedRenderFrameID });
	}

	void Renderer::RetireDrawSnapshots(uint64 frameID, std::shared_ptr<const void> mesh, std::shared_ptr<const void> material, std::shared_ptr<const void> skeleton)
	{
		// Keep replaced snapshots until every submission that could reference them has completed.
		if(_retiredDrawSnapshots.empty() || _retiredDrawSnapshots.back().frameID != frameID)
			_retiredDrawSnapshots.push_back({frameID, {}});

		auto &snapshots = _retiredDrawSnapshots.back().snapshots;
		if(mesh) snapshots.push_back(std::move(mesh));
		if(material) snapshots.push_back(std::move(material));
		if(skeleton) snapshots.push_back(std::move(skeleton));
	}

	void Renderer::DrainDrawSnapshots(uint64 completedFrameID)
	{
		while(!_retiredDrawSnapshots.empty() && _retiredDrawSnapshots.front().frameID <= completedFrameID)
			_retiredDrawSnapshots.pop_front();
	}

	void Renderer::FlushDeletedDrawables()
	{
		std::vector<Drawable *> drawables;
		{
			LockGuard<Lockable> lock(_frameLifecycleLock);
			size_t readyCount = 0;
			while(readyCount < _pendingDeletedDrawables.size() && _pendingDeletedDrawables[readyCount].frameID <= _completedRenderFrameID)
			{
				drawables.push_back(_pendingDeletedDrawables[readyCount].drawable);
				readyCount += 1;
			}

			_pendingDeletedDrawables.erase(_pendingDeletedDrawables.begin(), _pendingDeletedDrawables.begin() + readyCount);
		}

		for(Drawable *drawable : drawables)
			delete drawable;
	}

	void Renderer::FlushAllDeletedDrawables()
	{
		std::vector<Drawable *> drawables;
		{
			LockGuard<Lockable> lock(_frameLifecycleLock);
			for(const DeletedDrawable &deletedDrawable : _pendingDeletedDrawables)
				drawables.push_back(deletedDrawable.drawable);

			_pendingDeletedDrawables.clear();
		}

		for(Drawable *drawable : drawables)
			delete drawable;
	}

	void Renderer::ReleaseRendererReferences()
	{
		FlushAllDeletedDrawables();
		DrainDrawSnapshots(static_cast<uint64>(-1));

		{
			LockGuard<Lockable> lock(_rendererAttachmentsLock);
			_rendererAttachments.clear();
		}

		_cameraPassAttachmentSnapshotStack.clear();

		{
			LockGuard<Lockable> lock(_defaultShaderCacheLock);
			for(size_t typeIndex = 0; typeIndex < Shader::Type::COUNT; typeIndex++)
			{
				for(size_t hintIndex = 0; hintIndex < Shader::UsageHint::COUNT; hintIndex++)
				{
					SafeRelease(_defaultShaderCache[typeIndex][hintIndex]);
				}
			}
		}
	}

	void Renderer::PrintFrameStatistics(const RenderFrame &frame, float interval)
	{
		double currentTime = Kernel::GetSharedInstance()->GetTotalTime();
		if((currentTime - _frameStatisticsTimer) > interval)
		{
			_frameStatisticsTimer = currentTime;

			const std::vector<RenderFrame::CameraStatistics> &frameStatistics = frame.GetCameraStatistics();
			const size_t cameraCount = frameStatistics.size();
			uint64 totalVertices = 0;
			uint64 totalTriangles = 0;
			uint64 totalDrawables = 0;
			uint64 totalDrawCalls = 0;

			for(size_t i = 0; i < cameraCount; i++)
			{
				totalVertices += frameStatistics[i].numberOfVertices;
				totalTriangles += (frameStatistics[i].numberOfIndices / 3);
				totalDrawables += frameStatistics[i].numberOfDrawables;
				totalDrawCalls += frameStatistics[i].numberOfDrawCalls;
			}

			std::ostringstream statsStream;
			statsStream << "\nFrame stats | cameras=" << cameraCount
				<< " verts=" << totalVertices
				<< " tris=" << totalTriangles
				<< " drawables=" << totalDrawables
				<< " draws=" << totalDrawCalls;

			if(cameraCount > 1)
			{
				for(size_t i = 0; i < cameraCount; i++)
				{
					statsStream << "\n  cam " << i
						<< " | verts=" << frameStatistics[i].numberOfVertices
						<< " tris=" << (frameStatistics[i].numberOfIndices / 3)
						<< " drawables=" << frameStatistics[i].numberOfDrawables
						<< " draws=" << frameStatistics[i].numberOfDrawCalls;
				}
			}

			String *backendStatistics = GetBackendFrameStatistics();
			if(backendStatistics && backendStatistics->GetLength() > 0)
				statsStream << backendStatistics->GetUTF8String();

			RNInfo(statsStream.str());
		}
	}

	String *Renderer::GetBackendFrameStatistics() const
	{
		return nullptr;
	}

	void Renderer::ScheduleRenderThreadWork(Function &&function)
	{
		function();
	}

	void Renderer::SynchronizeRenderThread()
	{
	}

	void Renderer::WarmupDrawable(Mesh *mesh, Material *material, Camera *camera)
	{
	}

	Texture *Renderer::CreateTextureWithExternalMemory(const Texture::Descriptor &descriptor, const Texture::ExternalMemoryDescriptor &externalMemoryDescriptor)
	{
		RN_ASSERT(false, "External texture memory import is not supported by this renderer");
		return nullptr;
	}

	RenderPassResources *Renderer::CreateRenderPassResources()
	{
		return new RenderPassResources(this);
	}

	void Renderer::DeleteRenderPassResources(RenderPassResources *resources)
	{
		delete resources;
	}

	Shader *Renderer::GetDefaultShader(Shader::Type type, Shader::Options *options, Shader::UsageHint hint)
	{
		Shader::Options *realOptions = options ? options->Copy() : Shader::Options::WithNone()->Retain();

		if(hint == Shader::UsageHint::Multiview || hint == Shader::UsageHint::DepthMultiview || hint == Shader::UsageHint::ShadowDepthMultiview)
		{
			realOptions->EnableMultiview();
		}

		const size_t typeIndex = static_cast<size_t>(type);
		const size_t hintIndex = static_cast<size_t>(hint);
		RN_ASSERT(hintIndex < Shader::UsageHint::COUNT, "Invalid shader usage hint");

		{
			LockGuard<Lockable> lock(_defaultShaderCacheLock);
			Dictionary *cache = _defaultShaderCache[typeIndex][hintIndex];
			if(cache)
			{
				Shader *cachedShader = cache->GetObjectForKey<Shader>(realOptions);
				if(cachedShader)
				{
					realOptions->Release();
					return cachedShader;
				}
			}
		}

		ShaderLibrary *shaderLibrary = GetDefaultShaderLibrary();
		Shader *shader = nullptr;
		if(type == Shader::Type::Vertex)
		{
			if(hint == Shader::UsageHint::Depth || hint == Shader::UsageHint::ShadowDepth)
			{
				shader = shaderLibrary->GetShaderWithName(RNCSTR("depth_vertex"), realOptions);
			}
			else
			{
				if(realOptions && realOptions->HasValue("RN_SKY", "1")) //Use a different shader for the sky
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("sky_vertex"), realOptions);
				}
				else if(realOptions && realOptions->HasValue("RN_PARTICLES", "1"))
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("particles_vertex"), realOptions);
				}
				else if(realOptions && realOptions->HasValue("RN_UI", "1"))
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("ui_vertex"), realOptions);
				}
				else
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("gouraud_vertex"), realOptions);
				}
			}
		}
		else if(type == Shader::Type::Fragment)
		{
			if(hint == Shader::UsageHint::Depth || hint == Shader::UsageHint::ShadowDepth)
			{
				shader = shaderLibrary->GetShaderWithName(RNCSTR("depth_fragment"), realOptions);
			}
			else
			{
				if(realOptions && realOptions->HasValue("RN_SKY", "1")) //Use a different shader for the sky
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("sky_fragment"), realOptions);
				}
				else if(realOptions && realOptions->HasValue("RN_PARTICLES", "1"))
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("particles_fragment"), realOptions);
				}
				else if(realOptions && realOptions->HasValue("RN_UI", "1"))
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("ui_fragment"), realOptions);
				}
				else
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("gouraud_fragment"), realOptions);
				}
			}
		}

		if(shader)
		{
			LockGuard<Lockable> lock(_defaultShaderCacheLock);
			Dictionary *cache = _defaultShaderCache[typeIndex][hintIndex];
			if(!cache)
			{
				cache = new Dictionary();
				_defaultShaderCache[typeIndex][hintIndex] = cache;
			}

			Shader *cachedShader = cache->GetObjectForKey<Shader>(realOptions);
			if(cachedShader)
			{
				shader = cachedShader;
			}
			else
			{
				cache->SetObjectForKey(shader, realOptions);
			}
		}

		realOptions->Release();

		return shader;
	}
} // namespace RN
