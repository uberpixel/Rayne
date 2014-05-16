#Basic project setup
cmake_minimum_required(VERSION 2.6)
project(Rayne)

#Specify folder for custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/")

#Specify all source files to compile
#TODO: Maybe sort into groups?
set(RAYNE_SOURCES
	Rayne.h
	RNAABB.h
	RNAlgorithm.h
	RNAny.h
	RNApplication.cpp
	RNApplication.h
	RNArray.cpp
	RNArray.h
	RNASCIIString.cpp
	RNASCIIString.h
	RNAsset.cpp
	RNAsset.h
	RNAttributedString.cpp
	RNAttributedString.h
	RNAutoreleasePool.cpp
	RNAutoreleasePool.h
	RNBase.cpp
	RNBase.h
	RNBaseInternal.h
	RNBasicString.cpp
	RNBasicString.h
	RNBillboard.cpp
	RNBillboard.h
	RNCamera.cpp
	RNCamera.h
	RNCameraInternal.cpp
	RNCameraInternal.h
	RNCatalogue.cpp
	RNCatalogue.h
	RNColor.h
	RNConstants.h
	RNContextInternal.cpp
	RNContextInternal.h
	RNCountedSet.cpp
	RNCountedSet.h
	RNCPU.cpp
	RNCPU.h
	RNData.cpp
	RNData.h
	RNDataAttachment.cpp
	RNDataAttachment.h
	RNDebug.cpp
	RNDebug.h
	RNDecal.cpp
	RNDecal.h
	RNDefines.h
	RNDictionary.cpp
	RNDictionary.h
	RNEntity.cpp
	RNEntity.h
	RNEnum.h
	RNException.cpp
	RNException.h
	RNFile.cpp
	RNFile.h
	RNFileManager.cpp
	RNFileManager.h
	RNFormatter.cpp
	RNFormatter.h
	RNFunction.h
	RNGPUBuffer.cpp
	RNGPUBuffer.h
	RNHashTableInternal.h
	RNHit.h
	RNIndexPath.cpp
	RNIndexPath.h
	RNIndexSet.cpp
	RNIndexSet.h
	RNInput.cpp
	RNInput.h
	RNInstancingData.cpp
	RNInstancingData.h
	RNInstancingNode.cpp
	RNInstancingNode.h
	RNInterpolation.h
	RNIntervalTree.h
	RNJSONSerialization.cpp
	RNJSONSerialization.h
	RNKernel.cpp
	RNKernel.h
	RNKVO.cpp
	RNKVO.h
	RNKVOImplementation.h
	RNLight.cpp
	RNLight.h
	RNLightManager.cpp
	RNLightManager.h
	RNLockGuard.h
	RNLogging.cpp
	RNLogging.h
	RNLoggingEngine.cpp
	RNLoggingEngine.h
	RNMaterial.cpp
	RNMaterial.h
	RNMath.cpp
	RNMath.h
	RNMatrix.h
	RNMatrixQuaternion.h
	RNMemory.cpp
	RNMemory.h
	RNMesh.cpp
	RNMesh.h
	RNMessage.cpp
	RNMessage.h
	RNModel.cpp
	RNModel.h
	RNModule.cpp
	RNModule.h
	RNMutex.cpp
	RNMutex.h
	RNNull.cpp
	RNNull.h
	RNNumber.cpp
	RNNumber.h
	RNObject.cpp
	RNObject.h
	RNOctreeSceneManager.cpp
	RNOctreeSceneManager.h
	RNOpenGL.cpp
	RNOpenGL.h
	RNOpenGLBinder.cpp
	RNOpenGLQueue.cpp
	RNOpenGLQueue.h
	RNOpenPanel.cpp
	RNOpenPanel.h
	RNParticle.cpp
	RNParticle.h
	RNParticleEmitter.cpp
	RNParticleEmitter.h
	RNPathManager.cpp
	RNPathManager.h
	RNPlane.h
	RNPointGrid.cpp
	RNPointGrid.h
	RNProgress.cpp
	RNProgress.h
	RNQuaternion.h
	RNRandom.cpp
	RNRandom.h
	RNRect.h
	RNRenderer.cpp
	RNRenderer.h
	RNRenderer32.cpp
	RNRenderer32.h
	RNRenderStorage.cpp
	RNRenderStorage.h
	RNResourceCoordinator.cpp
	RNResourceCoordinator.h
	RNResourceLoader.cpp
	RNResourceLoader.h
	RNResourceLoaderBuiltIn.cpp
	RNResourceLoaderBuiltIn.h
	RNRingbuffer.h
	RNSceneManager.cpp
	RNSceneManager.h
	RNSceneNode.cpp
	RNSceneNode.h
	RNSceneNodeAttachment.cpp
	RNSceneNodeAttachment.h
	RNScopeGuard.h
	RNSculptable.cpp
	RNSculptable.h
	RNSemaphore.h
	RNSerialization.cpp
	RNSerialization.h
	RNSet.cpp
	RNSet.h
	RNSettings.cpp
	RNSettings.h
	RNSHA2.cpp
	RNSHA2.h
	RNShader.cpp
	RNShader.h
	RNShaderCache.cpp
	RNShaderCache.h
	RNShaderLookup.h
	RNShaderUnit.cpp
	RNShaderUnit.h
	RNSignal.h
	RNSIMD.h
	RNSingleton.h
	RNSkeleton.cpp
	RNSkeleton.h
	RNSpatialMap.h
	RNSphere.h
	RNSpinLock.cpp
	RNSpinLock.h
	RNSQLite3Internal.h
	RNStatistics.cpp
	RNStatistics.h
	RNSTL.h
	RNString.cpp
	RNString.h
	RNSyncPoint.cpp
	RNSyncPoint.h
	RNTerrain.cpp
	RNTerrain.h
	RNTextNode.cpp
	RNTextNode.h
	RNTexture.cpp
	RNTexture.h
	RNTextureAtlas.cpp
	RNTextureAtlas.h
	RNThread.cpp
	RNThread.h
	RNThreadLocalStorage.h
	RNThreadPool.cpp
	RNThreadPool.h
	RNTimer.cpp
	RNTimer.h
	RNTriggerZone.cpp
	RNTriggerZone.h
	RNTypeTranslator.h
	RNUIButton.cpp
	RNUIButton.h
	RNUIColor.cpp
	RNUIColor.h
	RNUIControl.cpp
	RNUIControl.h
	RNUIFont.cpp
	RNUIFont.h
	RNUIGeometry.h
	RNUIImage.cpp
	RNUIImage.h
	RNUIImageView.cpp
	RNUIImageView.h
	RNUILabel.cpp
	RNUILabel.h
	RNUIMenu.cpp
	RNUIMenu.h
	RNUIOutlineView.cpp
	RNUIOutlineView.h
	RNUIOutlineViewCell.cpp
	RNUIOutlineViewCell.h
	RNUIProgressIndicator.cpp
	RNUIProgressIndicator.h
	RNUIResponder.cpp
	RNUIResponder.h
	RNUIScroller.cpp
	RNUIScroller.h
	RNUIScrollerInternals.cpp
	RNUIScrollerInternals.h
	RNUIScrollView.cpp
	RNUIScrollView.h
	RNUISegmentView.cpp
	RNUISegmentView.h
	RNUIServer.cpp
	RNUIServer.h
	RNUIStyle.cpp
	RNUIStyle.h
	RNUITableView.cpp
	RNUITableView.h
	RNUITableViewCell.cpp
	RNUITableViewCell.h
	RNUITextEditor.cpp
	RNUITextEditor.h
	RNUITextField.cpp
	RNUITextField.h
	RNUITextStyle.h
	RNUITypesetter.cpp
	RNUITypesetter.h
	RNUIUtilities.cpp
	RNUIUtilities.h
	RNUIView.cpp
	RNUIView.h
	RNUIWidget.cpp
	RNUIWidget.h
	RNUIWidgetInternals.cpp
	RNUIWidgetInternals.h
	RNUnicode.cpp
	RNUnicode.h
	RNUTF8String.cpp
	RNUTF8String.h
	RNValue.cpp
	RNValue.h
	RNVector.h
	RNWater.cpp
	RNWater.h
	RNWindow.cpp
	RNWindow.h
	RNWindowInternal.cpp
	RNWindowInternal.h
	RNWorld.cpp
	RNWorld.h
	RNWorldAttachment.cpp
	RNWorldAttachment.h
	RNWorldCoordinator.cpp
	RNWorldCoordinator.h
	RNWrappingObject.h)


#Enable C++11
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

#Set include folders
set(RAYNE_INCLUDE_PATHS ${RAYNE_INCLUDE_PATHS}
	${CMAKE_CURRENT_SOURCE_DIR}
	"${CMAKE_CURRENT_SOURCE_DIR}/ogl")
#	"${CMAKE_BINARY_DIR}/Vendor/libjansson/jansson/include"
#	"${CMAKE_BINARY_DIR}/Vendor/zlib/zlib-1.2.8/include"
#	"${CMAKE_BINARY_DIR}/Vendor/libpng/lpng1610" "${CMAKE_SOURCE_DIR}/Vendor/libpng/lpng1610"
#	"${CMAKE_SOURCE_DIR}/Vendor/libfreetype/freetype-2.5.2/include")

#Force cmake to prefer static libraries
if(UNIX)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)# ${CMAKE_FIND_LIBRARY_SUFFIXES})
elseif(WIN32)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif(UNIX)


#Find OS independent dependencies:

#zlib
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
	set(RAYNE_INCLUDE_PATHS ${RAYNE_INCLUDE_PATHS} ${ZLIB_INCLUDE_DIRS})
	set(RAYNE_DEPENDEND_LIBRARIES ${RAYNE_DEPENDEND_LIBRARIES} ${ZLIB_LIBRARIES})
else(ZLIB_FOUND)
	MESSAGE(FATAL_ERROR "Library not found: ZLIB")
endif(ZLIB_FOUND)

#libpng
find_package(PNG REQUIRED)
if(PNG_FOUND)
	set(RAYNE_INCLUDE_PATHS ${RAYNE_INCLUDE_PATHS} ${PNG_INCLUDE_DIRS})
	set(RAYNE_DEPENDEND_LIBRARIES ${RAYNE_DEPENDEND_LIBRARIES} ${PNG_LIBRARIES})
else(PNG_FOUND)
	MESSAGE(FATAL_ERROR "Library not found: LIBPNG")
endif(PNG_FOUND)

#freetype
find_package(Freetype REQUIRED)
if(FREETYPE_FOUND)
	set(RAYNE_INCLUDE_PATHS ${RAYNE_INCLUDE_PATHS} ${FREETYPE_INCLUDE_DIRS})
	set(RAYNE_DEPENDEND_LIBRARIES ${RAYNE_DEPENDEND_LIBRARIES} ${FREETYPE_LIBRARIES})
else(FREETYPE_FOUND)
	MESSAGE(FATAL_ERROR "Library not found: FREETYPE")
endif(FREETYPE_FOUND)

#jansson
find_package(Jansson REQUIRED)
if(JANSSON_FOUND)
	set(RAYNE_INCLUDE_PATHS ${RAYNE_INCLUDE_PATHS} ${JANSSON_INCLUDE_DIRS})
	set(RAYNE_DEPENDEND_LIBRARIES ${RAYNE_DEPENDEND_LIBRARIES} ${JANSSON_LIBRARIES})
else(JANSSON_FOUND)
	MESSAGE(FATAL_ERROR "Library not found: JANSSON")
endif(JANSSON_FOUND)


if(UNIX)
	if(APPLE)
		#Make OSX specific setup
		
		#Activate objective-c++
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++ -mtune=core2")

		#Find libraries
		find_library(CORE_SERVICES_LIBRARY CoreServices)
		find_library(APP_KIT_LIBRARY AppKit)
		find_library(COCOA_LIBRARY Cocoa)
		find_library(CORE_DATA_LIBRARY CoreData)
		find_library(FOUNDATION_LIBRARY Foundation)
		find_library(IO_KIT_LIBRARY IOKit)
		find_library(OPENGL_LIBRARY OpenGL)

		#Find header paths
		find_path(CORE_SERVICES_INCLUDE_PATH CoreServices/CoreServices.h)
		find_path(APP_KIT_INCLUDE_PATH AppKit/AppKit.h)
		find_path(COCOA_INCLUDE_PATH Cocoa/Cocoa.h)
		find_path(CORE_DATA_INCLUDE_PATH CoreData/CoreData.h)
		find_path(FOUNDATION_INCLUDE_PATH Foundation/Foundation.h)
		find_path(IO_KIT_INCLUDE_PATH IOKit/IOKitLib.h)
		find_path(OPENGL_INCLUDE_PATH OpenGL/OpenGL.h)

		#Hide as advanced settings
		mark_as_advanced(
			CORE_SERVICES_LIBRARY CORE_SERVICES_INCLUDE_PATH
			APP_KIT_LIBRARY APP_KIT_INCLUDE_PATH
			COCOA_LIBRARY COCOA_INCLUDE_PATH
			CORE_DATA_LIBRARY CORE_DATA_INCLUDE_PATH
			FOUNDATION_LIBRARY FOUNDATION_INCLUDE_PATH
			IO_KIT_LIBRARY IO_KIT_INCLUDE_PATH
			OPENGL_LIBRARY OPENGL_INCLUDE_PATH)

		#Set library variable
		set(RAYNE_DEPENDEND_LIBRARIES ${RAYNE_DEPENDEND_LIBRARIES}
			${CORE_SERVICES_LIBRARY}
			${APP_KIT_LIBRARY}
			${COCOA_LIBRARY}
			${CORE_DATA_LIBRARY}
			${FOUNDATION_LIBRARY}
			${IO_KIT_LIBRARY}
			${OPENGL_LIBRARY})

		#Set include variable
		set(RAYNE_INCLUDE_PATHS ${RAYNE_INCLUDE_PATHS}
			${CORE_SERVICES_INCLUDE_PATH}
			${APP_KIT_INCLUDE_PATH}
			${COCOA_INCLUDE_PATH}
			${CORE_DATA_INCLUDE_PATH}
			${FOUNDATION_INCLUDE_PATH}
			${IO_KIT_INCLUDE_PATH}
			${OPENGL_INCLUDE_PATH})
	else(APPLE)
		#Make Linux specific setup
		
		#Activate SSE3
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
		
		#Find libraries
		find_library(XRANDR_LIBRARY NAMES Xrandr)

		#Find header paths
		find_path(XRANDR_INCLUDE_PATH X11/extensions/Xrandr.h)

		#Hide as advanced settings
		mark_as_advanced(
			XRANDR_LIBRARY XRANDR_INCLUDE_PATH)

		#Set library variable
		set(RAYNE_DEPENDEND_LIBRARIES ${RAYNE_DEPENDEND_LIBRARIES}
			${XRANDR_LIBRARY})

		#Set include variable
		set(RAYNE_INCLUDE_PATHS ${RAYNE_INCLUDE_PATHS}
			${XRANDR_INCLUDE_PATH})
	endif(APPLE)
endif(UNIX)

#Set include folders
include_directories(${RAYNE_INCLUDE_PATHS})

#Set architecture
set(CMAKE_OSX_ARCHITECTURES x86_64)

#Create a shared library
add_library(Rayne SHARED ${RAYNE_SOURCES})

#Set defines
set(RAYNE_DEFINITIONS DEBUG=1 RN_BUILD_LIBRARY=1)
set_target_properties(Rayne PROPERTIES COMPILE_DEFINITIONS "${RAYNE_DEFINITIONS}") 

#Set dependencies
target_link_libraries(Rayne ${RAYNE_DEPENDEND_LIBRARIES})

#MESSAGE(ERROR "${RAYNE_INCLUDE_PATHS}")
