# Find the Epiphan SDK 
# This module defines
# EPIPHAN_FOUND - EPIPHAN SDK has been found on this system
# EPIPHAN_INCLUDE_DIR - where to find the header files
# EPIPHAN_LIBRARY - libraries to be linked
# EPIPHAN_BINARY_DIR - shared libraries to be installed

FIND_PATH(EPIPHAN_INCLUDE_DIR frmgrab.h 
  PATHS ${CMAKE_CURRENT_BINARY_DIR}/src/ImageAcquisition/Epiphan/epiphan
  )

FIND_LIBRARY(EPIPHAN_LIBRARY frmgrab.lib
  PATHS ${CMAKE_CURRENT_BINARY_DIR}/src/ImageAcquisition/Epiphan/epiphan/Win32 
  )

FIND_PATH(EPIPHAN_BINARY_DIR frmgrab.dll
  PATHS ${CMAKE_CURRENT_BINARY_DIR}/src/ImageAcquisition/Epiphan/epiphan/ 
  )

# handle the QUIETLY and REQUIRED arguments and set EPIPHAN_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EPIPHAN DEFAULT_MSG 
  EPIPHAN_LIBRARY
  EPIPHAN_INCLUDE_DIR
  EPIPHAN_BINARY_DIR
  )