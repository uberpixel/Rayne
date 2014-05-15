# Try to find Rayne
# Once done this will define
#
#  RAYNE_FOUND - system has Rayne
#  RAYNE_INCLUDE_DIRS - the Rayne include directory
#  RAYNE_LIBRARIES - Link these to use Rayne


if(RAYNE_LIBRARIES AND RAYNE_INCLUDE_DIRS)
	#in cache already
	set(RAYNE_FOUND TRUE)
else(RAYNE_LIBRARIES AND RAYNE_INCLUDE_DIRS)
	find_path(RAYNE_INCLUDE_DIR
		NAMES
		Rayne.h
		PATHS
		/usr/include/rayne/Rayne
		/usr/local/include/rayne/Rayne
		/opt/local/include/rayne/Rayne
		/sw/include/rayne/Rayne
		/usr/include
		/usr/local/include
		/opt/local/include
		/sw/include)

	find_library(RAYNE_LIBRARY
		NAMES
		rayne
		PATHS
		/usr/lib
		/usr/local/lib
		/opt/local/lib
		/sw/lib)

	set(RAYNE_INCLUDE_DIRS ${RAYNE_INCLUDE_DIR})

	if(RAYNE_LIBRARY)
		set(RAYNE_LIBRARIES ${RAYNE_LIBRARIES} ${RAYNE_LIBRARY})
	endif(RAYNE_LIBRARY)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(Rayne DEFAULT_MSG RAYNE_LIBRARIES RAYNE_INCLUDE_DIRS)

	#hide variables in advanced view
	mark_as_advanced(RAYNE_INCLUDE_DIRS RAYNE_LIBRARIES)
endif(RAYNE_LIBRARIES AND RAYNE_INCLUDE_DIRS)
