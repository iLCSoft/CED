########################################################
# cmake file for building CED
# @author Jan Engels, Desy IT
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################


# project name
PROJECT( CED C )

# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 1 )
SET( ${PROJECT_NAME}_VERSION_MINOR 1 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )



### SETTINGS ################################################################

# try to load ilcsoft CMakeModules from CMAKE_MODULE_PATH
INCLUDE( ilcsoft_cmake_modules )
IF( NOT ILCSOFT_CMAKE_MODULES_FOUND )
    MESSAGE( FATAL_ERROR "Could not find ilcsoft CMakeModules in CMAKE_MODULE_PATH" )
ENDIF( NOT ILCSOFT_CMAKE_MODULES_FOUND )
ILCSOFT_CMAKE_MODULES_MINIMUM_REQUIRED( 0.11 )
INCLUDE( ilcsoft_default_settings )



### DEPENDENCIES ############################################################

# GLUT and OpenGL are required by glced
FIND_PACKAGE( GLUT QUIET )
FIND_PACKAGE( OpenGL QUIET )



### DOCUMENTATION ###########################################################

#OPTION( INSTALL_DOC "Set to OFF to skip build/install Documentation" ON )
#
#IF( INSTALL_DOC AND EXISTS "${PROJECT_SOURCE_DIR}/doc/CMakeLists.txt" )
#    ADD_SUBDIRECTORY( doc )
#ELSE()
#    MESSAGE( STATUS "INSTALL_DOC set to OFF" )
#    SET( INSTALL_DOC OFF )
#ENDIF()



### LIBRARY AND TOOLS #######################################################

# include directories
INCLUDE_DIRECTORIES( ./include )
INCLUDE_DIRECTORIES( ./src )
INSTALL_DIRECTORY( ./include DESTINATION . FILES_MATCHING PATTERN "*.h" )


# definitions to pass to the compiler
ADD_DEFINITIONS( "-Wall" ) # -ansi -pedantic



# --------- CED CLIENT LIBRARY ----------------------
AUX_SOURCE_DIRECTORY( ./src/client client_sources )
ADD_SHARED_LIBRARY( CED ${client_sources} )
INSTALL_SHARED_LIBRARY( CED DESTINATION lib )

# link CED client library to the math library
FIND_LIBRARY( c_math_lib NAMES m )
TARGET_LINK_LIBRARIES( CED ${c_math_lib} )
MARK_AS_ADVANCED( c_math_lib )



# --------- CED SERVER (GLCED) ----------------------
OPTION( CED_SERVER "Set to ON to build the glced server" OFF )
IF( CED_SERVER )

    IF( NOT GLUT_FOUND OR NOT OPENGL_FOUND )
        MESSAGE( FATAL_ERROR "could not find packages glut + opengl required to build glced" )
    ENDIF()

    INCLUDE_DIRECTORIES( ${GLUT_INCLUDE_DIR} )

    AUX_SOURCE_DIRECTORY( ./src/server server_sources )
    ADD_EXECUTABLE( glced ${server_sources} )

    TARGET_LINK_LIBRARIES( glced CED 
        ${GLUT_glut_LIBRARY}
        ${OPENGL_LIBRARIES}
    )

    INSTALL( TARGETS glced DESTINATION bin )

ENDIF( CED_SERVER )



# ------------------ CED TESTS ------------------
FOREACH( _testname test_ced test_ced_mhits )
    ADD_EXECUTABLE( ${_testname} ./src/tests/${_testname}.c )
    TARGET_LINK_LIBRARIES( ${_testname} CED )

    INSTALL( TARGETS ${_testname} DESTINATION bin )
ENDFOREACH()



# display some variables and write them to cache
DISPLAY_STD_VARIABLES()
MESSAGE( STATUS "CED_SERVER = ${CED_SERVER}" )
MESSAGE( STATUS )


# generate and install following configuration files
GENERATE_PACKAGE_CONFIGURATION_FILES( CEDConfig.cmake CEDConfigVersion.cmake CEDLibDeps.cmake )

