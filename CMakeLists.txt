####################################
# cmake file for building CED
# @author Jan Engels, DESY
####################################


########################################################
# CMake compatibility issues: don't modify this, please!
CMAKE_MINIMUM_REQUIRED( VERSION 2.4.6 )
#SET( CMAKE_BACKWARDS_COMPATIBILITY 2.4.6 )
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
# allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )
########################################################

# project name
PROJECT( CED C )

# project options
OPTION( BUILD_SHARED_LIBS "Set to OFF to build static libraries" ON )

# project version
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 1 )

# set default install prefix to project root directory
IF( CMAKE_INSTALL_PREFIX STREQUAL "/usr/local" )
    SET( CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}" )
ENDIF()

# library *nix style versioning
SET( ${PROJECT_NAME}_SOVERSION
    "${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}" )
SET( ${PROJECT_NAME}_VERSION
    "${${PROJECT_NAME}_SOVERSION}.${${PROJECT_NAME}_PATCH_LEVEL}" )

# add library install path to the rpath list
SET( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )
MARK_AS_ADVANCED( CMAKE_INSTALL_RPATH )
                                                                                                                                                            
# append link pathes to rpath list
SET( CMAKE_INSTALL_RPATH_USE_LINK_PATH 1 )
MARK_AS_ADVANCED( CMAKE_INSTALL_RPATH_USE_LINK_PATH )

# output directories
SET( EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin" CACHE PATH
    "EXECUTABLE_OUTPUT_PATH" FORCE )
SET( LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib" CACHE PATH
    "LIBRARY_OUTPUT_PATH" FORCE )
MARK_AS_ADVANCED( EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH )

# create uninstall configuration file
CONFIGURE_FILE( "${CMAKE_SOURCE_DIR}/cmake_uninstall.cmake.in"
                "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
                IMMEDIATE @ONLY )
                                                                                                                                                            
# create uninstall target
ADD_CUSTOM_TARGET( uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake" )
                                                                                                                                                            
# create configuration file from .in file
CONFIGURE_FILE( "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in"
                "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY )
                                                                                                                                                            
# install configuration file
INSTALL( FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake" DESTINATION . )

# install include files
INSTALL( FILES "${CMAKE_SOURCE_DIR}/ced.h" "${CMAKE_SOURCE_DIR}/ced_cli.h"
        DESTINATION . )
INSTALL( FILES "${CMAKE_SOURCE_DIR}/ced.h" "${CMAKE_SOURCE_DIR}/ced_cli.h"
        DESTINATION include )

###################################################################################

# GLUT is required for building CED
FIND_PACKAGE( GLUT REQUIRED )

# MAC ONLY
IF(APPLE)
FIND_PACKAGE( OpenGL REQUIRED )
ENDIF(APPLE)

# include directories
INCLUDE_DIRECTORIES( . )
INCLUDE_DIRECTORIES( ${GLUT_INCLUDE_DIR} )

# add definitions
ADD_DEFINITIONS( "-Wall" )

# LIBRARY
ADD_LIBRARY( lib_${PROJECT_NAME} ced_srv.c ced.c glut_socks.c )

ADD_CUSTOM_TARGET( lib DEPENDS lib_${PROJECT_NAME} )
SET_TARGET_PROPERTIES( lib_${PROJECT_NAME} PROPERTIES
    # create *nix style library versions + symbolic links
    VERSION ${${PROJECT_NAME}_VERSION}
    SOVERSION ${${PROJECT_NAME}_SOVERSION}
    # allow creating static and shared libs without conflicts
    CLEAN_DIRECT_OUTPUT 1
    # avoid conflicts between library and binary target names
    OUTPUT_NAME ${PROJECT_NAME} )

TARGET_LINK_LIBRARIES( lib_${PROJECT_NAME} ${GLUT_glut_LIBRARY} )
#MAC ONLY
IF(APPLE)
TARGET_LINK_LIBRARIES( lib_${PROJECT_NAME} ${OPENGL_LIBRARIES} )
ENDIF(APPLE)

# install library
INSTALL( TARGETS lib_${PROJECT_NAME} DESTINATION lib PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE )

# BINARY
ADD_EXECUTABLE( bin glced.c )
SET_TARGET_PROPERTIES( bin PROPERTIES OUTPUT_NAME glced )
TARGET_LINK_LIBRARIES( bin lib_${PROJECT_NAME} )
INSTALL( TARGETS bin DESTINATION . )
INSTALL( TARGETS bin DESTINATION bin )


ADD_EXECUTABLE( test_ced test_ced.c ced_cli.c ced.c)
INSTALL( TARGETS test_ced DESTINATION bin )
#TARGET_LINK_LIBRARIES( test_ced lib_${PROJECT_NAME} )



###################################################################################

# display status message for important variables
MESSAGE( STATUS )
MESSAGE( STATUS "-------------------------------------------------------------------------------" )
MESSAGE( STATUS "BUILD_SHARED_LIBS = ${BUILD_SHARED_LIBS}" )
MESSAGE( STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )
MESSAGE( STATUS "Change a value with: cmake -D<Variable>=<Value>" )
MESSAGE( STATUS "-------------------------------------------------------------------------------" )
MESSAGE( STATUS )

# force some variables that could be defined in the command line
# to be written to cache
SET( BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}" CACHE BOOL
    "Set to OFF to build static libraries" FORCE )
SET( CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH
    "Where to install ${PROJECT_NAME}" FORCE )

# export build settings
INCLUDE( CMakeExportBuildSettings )
CMAKE_EXPORT_BUILD_SETTINGS( "${PROJECT_NAME}BuildSettings.cmake" )

# export library dependencies (keep this as the last line in the file)
EXPORT_LIBRARY_DEPENDENCIES( "${PROJECT_NAME}LibDeps.cmake" )
