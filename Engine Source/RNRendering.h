//
//  RNRendering.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERING_H__
#define __RAYNE_RENDERING_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSpinLock.h"
#include "RNRenderingResource.h"

#include "RNCamera.h"
#include "RNVector.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNTexture.h"
#include "RNMaterial.h"
#include "RNMesh.h"

namespace RN
{
	class RenderingProxy
	{
	public:
		typedef enum
		{
			TypeTexture,
			TypeMaterial,
			TypeShader,
			TypeMesh
		} Type;
		
		RenderingProxy(Type _type)
		{
			type   = _type;
			_inUse = false;
			_next  = _prev = 0;
		}
		
		~RenderingProxy()
		{
			if(_prev)
				_prev->_next = _next;
			
			if(_next)
				_next->_prev = _prev;
			
			if(_inUse)
				FreeData();
		}
		
		
		virtual RenderingProxy *CreateProxy()
		{
			return new RenderingProxy(type);
		}
		
		virtual void FreeData()
		{
		}
		
		
		RenderingProxy *FreeProxy()
		{
			RenderingProxy *proxy = this;
			while(proxy->_prev)
				proxy = proxy->_prev;
			
			while(proxy)
			{
				if(!proxy->_next)
					break;
				
				proxy->Lock();
				
				if(!proxy->_inUse)
				{
					proxy->_inUse = true;
					proxy->Unlock();
					
					return proxy;
				}
				
				proxy->Unlock();
				proxy = proxy->_next;
			}
			
			proxy->_next = CreateProxy();
			proxy->_next->_prev = proxy;
			
			proxy = proxy->_next;
			proxy->_inUse = true;
			
			return proxy;
		}
		
		void Relinquish()
		{
			Lock();
			
			_inUse = false;
			FreeData();
			
			Unlock();
		}
		
		
		
		void Lock()
		{
			_lock.Lock();
		}
		
		void Unlock()
		{
			_lock.Unlock();
		}
		
		Type type;
		
	private:
		SpinLock _lock;
		bool _inUse;
		
		RenderingProxy *_next;
		RenderingProxy *_prev;
	};
	
	class RenderingIntent : public RenderingResource
	{
	public:
		RenderingIntent() :
			RenderingResource("Rendering Intent")
		{
		}
		
		Matrix transform;
		Material *material;
		Mesh *mesh;
	};
}

#endif /* __RAYNE_RENDERING_H__ */
