//
//  RNShadowVolume.h
//  Rayne
//
//  Copyright 2010 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SHADOWVOLUME_H__
#define __RAYNE_SHADOWVOLUME_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Assets/RNAsset.h"
#include "../Math/RNMatrix.h"
#include "../Math/RNQuaternion.h"
#include "../Math/RNVector.h"
#include "../Scene/RNLight.h"

namespace RN
{
	class Model;
	class Mesh;
	
	/**
	 * Shadow volume edge class. Represents an edge as it is needed to generate the shadow volume.
	 */
	class ShadowVolumeEdge
	{
		public:
			bool operator== (const ShadowVolumeEdge &other);
		
			Vector3 position[2];
			unsigned int counter[2];
			bool lastaddor;
	};

	/**
	 * Shadow volume face class. Represents a face as it is needed to generate the shadow volume.
	 */
	class ShadowVolumeFace
	{
		public:
			unsigned int edge[3];
			bool flipped[3];
			Vector3 normal;
			Vector3 position;
	};

	/**
	 * Shadow volume class. Generates and represents a shadow volume needed for stencil shadows.
	 */
	class ShadowVolume : public Asset
	{
		public:
			ShadowVolume();
			~ShadowVolume();
		
			void SetModel(Model *model, unsigned int lod = 0);
		
			Mesh *GenerateMesh() const;
			void UpdateMesh(SceneNode *node, Mesh *mesh, Light *light);
		
		private:
			void CalculateFaceNormal(ShadowVolumeFace *face, const Vector3 &position1, const Vector3 &position2, const Vector3 &position3);
		
			unsigned int _faceCount;
			unsigned int _edgeCount;
			ShadowVolumeFace *_faces;
			ShadowVolumeEdge *_edges;
		
			__RNDeclareMetaInternal(ShadowVolume)
	};
}

#endif //__RAYNE_SHADOWVOLUME_H__
