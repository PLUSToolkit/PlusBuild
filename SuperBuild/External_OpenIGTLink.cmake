IF(OpenIGTLink_DIR)
  # OpenIGTLink has been built already
  FIND_PACKAGE(OpenIGTLink REQUIRED NO_MODULE)
  IF(${OpenIGTLink_PROTOCOL_VERSION} LESS 3)
    MESSAGE(FATAL_ERROR "PLUS requires a build of OpenIGTLink with v3 support enabled. Please point OpenIGTLink_DIR to an implementation with v3 support.")
  ENDIF()

  MESSAGE(STATUS "Using OpenIGTLink available at: ${OpenIGTLink_DIR}")
  
  # Copy libraries to CMAKE_RUNTIME_OUTPUT_DIRECTORY
  IF(MSVC OR ${CMAKE_GENERATOR} MATCHES "Xcode")
    FILE(COPY 
      ${OpenIGTLink_LIBRARY_DIRS}/Release/
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release
      FILES_MATCHING REGEX .*${CMAKE_SHARED_LIBRARY_SUFFIX}
      )
    FILE(COPY 
      ${OpenIGTLink_LIBRARY_DIRS}/Debug/
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug
      FILES_MATCHING REGEX .*${CMAKE_SHARED_LIBRARY_SUFFIX}
      )    
  ELSE()
    FILE(COPY 
      ${OpenIGTLink_LIBRARY_DIRS}/
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
      FILES_MATCHING REGEX .*${CMAKE_SHARED_LIBRARY_SUFFIX}
      )
  ENDIF()

  SET (PLUS_OpenIGTLink_DIR "${OpenIGTLink_DIR}" CACHE INTERNAL "Path to store OpenIGTLink binaries")
ELSE()
  # OpenIGTLink has not been built yet, so download and build it as an external project
  SetGitRepositoryTag(
    OpenIGTLink
    "${GIT_PROTOCOL}://github.com/openigtlink/OpenIGTLink.git"
    "master"
    )

  SET (PLUS_OpenIGTLink_SRC_DIR "${CMAKE_BINARY_DIR}/Deps/OpenIGTLink")
  SET (PLUS_OpenIGTLink_DIR "${CMAKE_BINARY_DIR}/Deps/OpenIGTLink-bin" CACHE INTERNAL "Path to store OpenIGTLink binaries")
  ExternalProject_Add( OpenIGTLink
    "${PLUSBUILD_EXTERNAL_PROJECT_CUSTOM_COMMANDS}"
    PREFIX "${CMAKE_BINARY_DIR}/Deps/OpenIGTLink-prefix"
    SOURCE_DIR "${PLUS_OpenIGTLink_SRC_DIR}"
    BINARY_DIR "${PLUS_OpenIGTLink_DIR}"
    #--Download step--------------
    GIT_REPOSITORY ${OpenIGTLink_GIT_REPOSITORY}
    GIT_TAG ${OpenIGTLink_GIT_TAG}
    #--Configure step-------------
    CMAKE_ARGS 
      ${ep_common_args}
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
      -DBUILD_SHARED_LIBS:BOOL=${PLUSBUILD_BUILD_SHARED_LIBS}
      -DBUILD_EXAMPLES:BOOL=OFF
      -DBUILD_TESTING:BOOL=OFF
      -DOpenIGTLink_PROTOCOL_VERSION_2:BOOL=OFF
      -DOpenIGTLink_PROTOCOL_VERSION_3:BOOL=ON
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
    #--Build step-----------------
    BUILD_ALWAYS 1
    #--Install step-----------------
    INSTALL_COMMAND ""
    DEPENDS ${OpenIGTLink_DEPENDENCIES}
    )  
ENDIF()