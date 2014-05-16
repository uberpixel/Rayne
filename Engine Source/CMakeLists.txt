#Basic project setup
cmake_minimum_required(VERSION 2.6)
project(Rayne)

#Specify folder for custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/")

#Specify all source files to compile
#TODO: Maybe sort into groups?
set(RAYNE_SOURCES
	RNApplication.cpp
	RNArray.cpp
	RNASCIIString.cpp
	RNAsset.cpp
	RNAttributedString.cpp
	RNAutoreleasePool.cpp
	RNBase.cpp
	RNBasicString.cpp
	RNBillboard.cpp
	RNCamera.cpp
	RNCameraInternal.cpp
	RNCatalogue.cpp
	RNContextInternal.cpp
	RNCountedSet.cpp
	RNCPU.cpp
	RNData.cpp
	RNDataAttachment.cpp
	RNDebug.cpp
	RNDecal.cpp
	RNDictionary.cpp
	RNEntity.cpp
	RNException.cpp
	RNFile.cpp
	RNFileManager.cpp
	RNFormatter.cpp
	RNGPUBuffer.cpp
	RNIndexPath.cpp
	RNIndexSet.cpp
	RNInput.cpp
	RNInstancingData.cpp
	RNInstancingNode.cpp
	RNJSONSerialization.cpp
	RNKernel.cpp
	RNKVO.cpp
	RNLight.cpp
	RNLightManager.cpp
	RNLogging.cpp
	RNLoggingEngine.cpp
	RNMaterial.cpp
	RNMath.cpp
	RNMemory.cpp
	RNMesh.cpp
	RNMessage.cpp
	RNModel.cpp
	RNModule.cpp
	RNMutex.cpp
	RNNull.cpp
	RNNumber.cpp
	RNObject.cpp
	RNOctreeSceneManager.cpp
	RNOpenGL.cpp
	RNOpenGLBinder.cpp
	RNOpenGLQueue.cpp
	RNOpenPanel.cpp
	RNParticle.cpp
	RNParticleEmitter.cpp
	RNPathManager.cpp
	RNPointGrid.cpp
	RNProgress.cpp
	RNRandom.cpp
	RNRenderer.cpp
	RNRenderer32.cpp
	RNRenderStorage.cpp
	RNResourceCoordinator.cpp
	RNResourceLoader.cpp
	RNResourceLoaderBuiltIn.cpp
	RNSceneManager.cpp
	RNSceneNode.cpp
	RNSceneNodeAttachment.cpp
	RNSculptable.cpp
	RNSerialization.cpp
	RNSet.cpp
	RNSettings.cpp
	RNSHA2.cpp
	RNShader.cpp
	RNShaderCache.cpp
	RNShaderUnit.cpp
	RNSkeleton.cpp
	RNSpinLock.cpp
	RNStatistics.cpp
	RNString.cpp
	RNSyncPoint.cpp
	RNTerrain.cpp
	RNTextNode.cpp
	RNTexture.cpp
	RNTextureAtlas.cpp
	RNThread.cpp
	RNThreadPool.cpp
	RNTimer.cpp
	RNTriggerZone.cpp
	RNUIButton.cpp
	RNUIColor.cpp
	RNUIControl.cpp
	RNUIFont.cpp
	RNUIImage.cpp
	RNUIImageView.cpp
	RNUILabel.cpp
	RNUIMenu.cpp
	RNUIOutlineView.cpp
	RNUIOutlineViewCell.cpp
	RNUIProgressIndicator.cpp
	RNUIResponder.cpp
	RNUIScroller.cpp
	RNUIScrollerInternals.cpp
	RNUIScrollView.cpp
	RNUISegmentView.cpp
	RNUIServer.cpp
	RNUIStyle.cpp
	RNUITableView.cpp
	RNUITableViewCell.cpp
	RNUITextEditor.cpp
	RNUITextField.cpp
	RNUITypesetter.cpp
	RNUIUtilities.cpp
	RNUIView.cpp
	RNUIWidget.cpp
	RNUIWidgetInternals.cpp
	RNUnicode.cpp
	RNUTF8String.cpp
	RNValue.cpp
	RNWater.cpp
	RNWindow.cpp
	RNWindowInternal.cpp
	RNWorld.cpp
	RNWorldAttachment.cpp
	RNWorldCoordinator.cpp)


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
