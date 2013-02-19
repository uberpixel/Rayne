//
//  RNOpenGLExtensions.h
//  Rayne
//
//  Created by Sidney Just on 19.02.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __RAYNE_OPENGLEXTENSIONS_H__
#define __RAYNE_OPENGLEXTENSIONS_H__

#include "RNBase.h"

// ----
// EXT Extensions
// ----

// GL_EXT_shadow_samplers
#if !defined(GL_TEXTURE_COMPARE_MODE) && defined(GL_TEXTURE_COMPARE_MODE_EXT)
#define GL_TEXTURE_COMPARE_MODE GL_TEXTURE_COMPARE_MODE_EXT
#endif

#if !defined(GL_TEXTURE_COMPARE_FUNC) && defined(GL_TEXTURE_COMPARE_FUNC_EXT)
#define GL_TEXTURE_COMPARE_FUNC GL_TEXTURE_COMPARE_FUNC_EXT
#endif

#if !defined(GL_COMPARE_REF_TO_TEXTURE) && defined(GL_COMPARE_REF_TO_TEXTURE_EXT)
#define GL_COMPARE_REF_TO_TEXTURE GL_COMPARE_REF_TO_TEXTURE_EXT
#endif

#if !defined(GL_SAMPLER_2D_SHADOW) && defined(GL_SAMPLER_2D_SHADOW_EXT)
#define GL_SAMPLER_2D_SHADOW GL_SAMPLER_2D_SHADOW_EXT
#endif

// GL_EXT_texture_storage

#if !defined(GL_TEXTURE_IMMUTABLE_FORMAT) && defined(GL_TEXTURE_IMMUTABLE_FORMAT_EXT)
#define GL_TEXTURE_IMMUTABLE_FORMAT GL_TEXTURE_IMMUTABLE_FORMAT_EXT
#endif

#if !defined(GL_ALPHA8) && defined(GL_ALPHA8_EXT)
#define GL_ALPHA8 GL_ALPHA8_EXT
#endif

#if !defined(GL_LUMINANCE8) && defined(GL_LUMINANCE8_EXT)
#define GL_LUMINANCE8 GL_LUMINANCE8_EXT
#endif

#if !defined(GL_LUMINANCE8_ALPHA8) && defined(GL_LUMINANCE8_ALPHA8_EXT)
#define GL_LUMINANCE8_ALPHA8 GL_LUMINANCE8_ALPHA8_EXT
#endif

#if !defined(GL_BGRA8) && defined(GL_BGRA8_EXT)
#define GL_BGRA8 GL_BGRA8_EXT
#endif

#if !defined(GL_RGBA32F) && defined(GL_RGBA32F_EXT)
#define GL_RGBA32F GL_RGBA32F_EXT
#endif

#if !defined(GL_RGB32F) && defined(GL_RGB32F_EXT)
#define GL_RGB32F GL_RGB32F_EXT
#endif

#if !defined(GL_RG32F) && defined(GL_RG32F_EXT)
#define GL_RG32F GL_RG32F_EXT
#endif

#if !defined(GL_R32F) && defined(GL_R32F_EXT)
#define GL_R32F GL_R32F_EXT
#endif

#if !defined(GL_ALPHA32F) && defined(GL_ALPHA32F_EXT)
#define GL_ALPHA32F GL_ALPHA32F_EXT
#endif

#if !defined(GL_LUMINANCE32F) && defined(GL_LUMINANCE32F_EXT)
#define GL_LUMINANCE32F GL_LUMINANCE32F_EXT
#endif

#if !defined(GL_LUMINANCE_ALPHA32F) && defined(GL_LUMINANCE_ALPHA32F_EXT)
#define GL_LUMINANCE_ALPHA32F GL_LUMINANCE_ALPHA32F_EXT
#endif

#if !defined(GL_ALPHA16F) && defined(GL_ALPHA16F_EXT)
#define GL_ALPHA16F GL_ALPHA16F_EXT
#endif

#if !defined(GL_LUMINANCE16F) && defined(GL_LUMINANCE16F_EXT)
#define GL_LUMINANCE16F GL_LUMINANCE16F_EXT
#endif

#if !defined(GL_LUMINANCE_ALPHA16F) && defined(GL_LUMINANCE_ALPHA16F_EXT)
#define GL_LUMINANCE_ALPHA16F GL_LUMINANCE_ALPHA16F_EXT
#endif

// GL_EXT_texture_filter_anisotropic

#if !defined(GL_TEXTURE_MAX_ANISOTROPY) && defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
#define GL_TEXTURE_MAX_ANISOTROPY GL_TEXTURE_MAX_ANISOTROPY_EXT
#endif

#if !defined(GL_MAX_TEXTURE_MAX_ANISOTROPY) && defined(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)
#define GL_MAX_TEXTURE_MAX_ANISOTROPY GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#endif

// GL_EXT_texture_rg
#if !defined(GL_RED) && defined(GL_RED_EXT)
#define GL_RED GL_RED_EXT
#endif

#if !defined(GL_RG) && defined(GL_RG_EXT)
#define GL_RG GL_RG_EXT
#endif

#if !defined(GL_R8) && defined(GL_R8_EXT)
#define GL_R8 GL_R8_EXT
#endif

#if !defined(GL_RG8) && defined(GL_RG8_EXT)
#define GL_RG8 GL_RG8_EXT
#endif

// GL_EXT_color_buffer_half_float
#if !defined(GL_RGBA16F) && defined(GL_RGBA16F_EXT)
#define GL_RGBA16F GL_RGBA16F_EXT
#endif

#if !defined(GL_RGB16F) && defined(GL_RGB16F_EXT)
#define GL_RGB16F GL_RGB16F_EXT
#endif

#if !defined(GL_RG16F) && defined(GL_RG16F_EXT)
#define GL_RG16F GL_RG16F_EXT
#endif

#if !defined(GL_R16F) && defined(GL_R16F_EXT)
#define GL_R16F GL_R16F_EXT
#endif

// ----
// OES Extensions
// ----

// GL_OES_depth24
#if !defined(GL_DEPTH_COMPONENT24) && defined(GL_DEPTH_COMPONENT24_OES)
#define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
#endif

// GL_OES_packed_depth_stencil
#if !defined(GL_DEPTH_STENCIL) && defined(GL_DEPTH_STENCIL_OES)
#define GL_DEPTH_STENCIL GL_DEPTH_STENCIL_OES
#endif

#if !defined(GL_UNSIGNED_INT_24_8) && defined(GL_UNSIGNED_INT_24_8_OES)
#define GL_UNSIGNED_INT_24_8 GL_UNSIGNED_INT_24_8_OES
#endif

#if !defined(GL_DEPTH24_STENCIL8) && defined(GL_DEPTH24_STENCIL8_OES)
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#endif

// GL_OES_texture_half_float

#if !defined(GL_HALF_FLOAT) && defined(GL_HALF_FLOAT_OES)
#define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#endif

#endif /* __RAYNE_OPENGLEXTENSIONS_H__ */
