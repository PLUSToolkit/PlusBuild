IF(IGSIO_DIR)
  # IGSIO has been built already
  FIND_PACKAGE(IGSIO REQUIRED NO_MODULE)
  MESSAGE(STATUS "Using IGSIO available at: ${IGSIO_DIR}")

  # Copy libraries to CMAKE_RUNTIME_OUTPUT_DIRECTORY
  PlusCopyLibrariesToDirectory(${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${IGSIO_LIBRARIES})

  SET (PLUS_IGSIO_DIR "${IGSIO_DIR}" CACHE INTERNAL "Path to store IGSIO binaries")
ELSE()

  SetGitRepositoryTag(
    IGSIO
    "https://github.com/IGSIO/IGSIO.git"
    "master"
    )

  SET(PLUSBUILD_ADDITIONAL_SDK_ARGS)

  IF(PLUSBUILD_BUILD_PLUSLIB_WIDGETS AND (${PLUSBUILD_VTK_VERSION} GREATER 8))
    # If VTK9 is configured Qt enabled then we need to provide Qt5_DIR
    LIST(APPEND PLUSBUILD_ADDITIONAL_SDK_ARGS
      -DQt5_DIR:PATH=${Qt5_DIR}
      )
  ENDIF()

  SET(PLUS_IGSIO_SRC_DIR ${CMAKE_BINARY_DIR}/IGSIO CACHE INTERNAL "Path to store IGSIO contents.")
  SET(PLUS_IGSIO_PREFIX_DIR ${CMAKE_BINARY_DIR}/IGSIO-prefix CACHE INTERNAL "Path to store IGSIO prefix data.")
  SET(PLUS_IGSIO_DIR ${CMAKE_BINARY_DIR}/IGSIO-bin/inner-build CACHE INTERNAL "Path to store IGSIO binaries")

  IF(TARGET vtk)
    LIST(APPEND IGSIO_DEPENDENCIES vtk)
  ENDIF()
  IF(TARGET itk)
    LIST(APPEND IGSIO_DEPENDENCIES itk)
  ENDIF()

  SET(IGSIO_BUILD_OPTIONS
    -DIGSIO_ZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
    -DIGSIO_ZLIB_LIBRARY:PATH=${ZLIB_LIBRARY}
    -DIGSIO_SUPERBUILD:BOOL=ON
    -DIGSIO_USE_3DSlicer:BOOL=OFF
    -DIGSIO_BUILD_SEQUENCEIO:BOOL=ON
    -DIGSIO_BUILD_VOLUMERECONSTRUCTION:BOOL=ON
    -DIGSIO_SEQUENCEIO_ENABLE_MKV:BOOL=${PLUS_USE_MKV_IO}
    -DIGSIO_USE_VP9:BOOL=${PLUS_USE_VP9}
    ${PLUSBUILD_ADDITIONAL_SDK_ARGS}
  )

  ExternalProject_Add( IGSIO
    PREFIX ${PLUS_IGSIO_PREFIX_DIR}
    "${PLUSBUILD_EXTERNAL_PROJECT_CUSTOM_COMMANDS}"
    SOURCE_DIR ${PLUS_IGSIO_SRC_DIR}
    BINARY_DIR ${CMAKE_BINARY_DIR}/IGSIO-bin
    #--Download step--------------
    GIT_REPOSITORY ${IGSIO_GIT_REPOSITORY}
    GIT_TAG ${IGSIO_GIT_TAG}
    #--Configure step-------------
    CMAKE_ARGS
      ${ep_common_args}
      -DEXECUTABLE_OUTPUT_PATH:PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DBUILD_SHARED_LIBS:BOOL=${PLUSBUILD_BUILD_SHARED_LIBS}
      -DVTK_DIR:PATH=${PLUS_VTK_DIR}
      -DITK_DIR:PATH=${PLUS_ITK_DIR}
      -DBUILD_TESTING:BOOL=OFF
      ${IGSIO_BUILD_OPTIONS}
    #--Build step-----------------
    BUILD_ALWAYS 1
    #--Install step-----------------
    INSTALL_COMMAND ""
    DEPENDS ${IGSIO_DEPENDENCIES}
    )
ENDIF()
