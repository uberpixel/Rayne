# - Find VTune ittnotify.
# Defines:
# VTune_FOUND
# VTune_INCLUDE_DIRS
# VTune_LIBRARY

set(VTune_DIRS
        "$ENV{VTUNE_AMPLIFIER_XE_2017_DIR}/"
        "$ENV{VTUNE_AMPLIFIER_XE_2016_DIR}/"
        "$ENV{VTUNE_AMPLIFIER_XE_2015_DIR}/"
        "$ENV{VTUNE_AMPLIFIER_XE_2013_DIR}/"
        "$ENV{VTUNE_AMPLIFIER_XE_2011_DIR}/")

find_path(VTune_INCLUDE_DIRS ittnotify.h
        PATHS ${VTune_DIRS}
        PATH_SUFFIXES include)

find_library(VTune_LIBRARY libittnotify
        HINTS "${VTune_INCLUDE_DIRS}/.."
        PATHS ${VTune_DIRS}
        PATH_SUFFIXES lib64)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VTune DEFAULT_MSG VTune_LIBRARY VTune_INCLUDE_DIRS)
