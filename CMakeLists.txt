########################################################
# cmake file for building CED
# @author Jan Engels, Desy IT
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################


# project name
PROJECT( CED )

# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 1 )
SET( ${PROJECT_NAME}_VERSION_MINOR 5 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )



### DEPENDENCIES ############################################################

FIND_PACKAGE( ILCUTIL COMPONENTS ILCSOFT_CMAKE_MODULES REQUIRED )

# GLUT and OpenGL are required by glced
FIND_PACKAGE( GLUT QUIET )
FIND_PACKAGE( OpenGL QUIET )

# load default settings from ILCSOFT_CMAKE_MODULES
INCLUDE( ilcsoft_default_settings )



### DOCUMENTATION ###########################################################

OPTION( INSTALL_DOC "Set to OFF to skip build/install Documentation" OFF )

IF( INSTALL_DOC AND EXISTS "${PROJECT_SOURCE_DIR}/doc/CMakeLists.txt" )
    ADD_SUBDIRECTORY( doc )
ELSE()
    MESSAGE( STATUS "INSTALL_DOC set to OFF" )
    SET( INSTALL_DOC OFF )
ENDIF()



### LIBRARY AND TOOLS #######################################################

# include directories
INCLUDE_DIRECTORIES( BEFORE ./include )
INCLUDE_DIRECTORIES( BEFORE ./src )
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
OPTION( CED_SERVER "Set to ON to build the glced server" ON )
MESSAGE( STATUS "CED_SERVER = ${CED_SERVER}" )

IF( CED_SERVER )

    IF( NOT GLUT_FOUND OR NOT OPENGL_FOUND )
        MESSAGE( FATAL_ERROR "could not find packages glut + opengl required to build glced" )
    ENDIF()

    INCLUDE_DIRECTORIES( ${GLUT_INCLUDE_DIR} )

    # copy paths from OPENGL_LIBRARIES to rpath list of glced to avoid system
    # opengl libraries used in the linking process of glced to be overwriten
    # by LD_LIBRARY_PATH, e.g.: LD_LIBRARY_PATH set by geant4 env_init_script
    # points to old mesa libraries in afs
    FOREACH( _opengl_lib ${OPENGL_LIBRARIES} )
        GET_FILENAME_COMPONENT( _opengl_lib_path ${_opengl_lib} PATH )
        LIST( APPEND _opengl_linker_paths ${_opengl_lib_path} )
    ENDFOREACH()

    # remove duplicate paths
    IF( _opengl_linker_paths )
        LIST( REMOVE_DUPLICATES _opengl_linker_paths )
    ENDIF()

    #MESSAGE( "_opengl_linker_paths: ${_opengl_linker_paths}" )

    # this would affect all targets
    #LIST( APPEND CMAKE_INSTALL_RPATH "${_opengl_linker_paths}" )
    #IF( APPLE )
    #    LIST( APPEND CMAKE_INSTALL_NAME_DIR "${_opengl_linker_paths}" )
    #ENDIF()

    AUX_SOURCE_DIRECTORY( ./src/server server_sources )
    ADD_EXECUTABLE( glced ${server_sources} )

    SET_TARGET_PROPERTIES( glced PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${_opengl_linker_paths}"
        INSTALL_NAME_DIR "${CMAKE_INSTALL_NAME_DIR};${_opengl_linker_paths}"
    )

    TARGET_LINK_LIBRARIES( glced CED 
        ${GLUT_glut_LIBRARY}
        ${OPENGL_LIBRARIES}
    )

    INSTALL( TARGETS glced DESTINATION bin )

ENDIF( CED_SERVER )


# ------------------ CED TESTS ------------------
FOREACH( _testname test_ced test_ced_mhits )

    IF( BUILD_TESTING )
        ADD_EXECUTABLE( ${_testname} ./src/tests/${_testname}.cc )
    ELSE()
        ADD_EXECUTABLE( ${_testname} EXCLUDE_FROM_ALL ./src/tests/${_testname}.cc )
    ENDIF()

    TARGET_LINK_LIBRARIES( ${_testname} CED )

    INSTALL( TARGETS ${_testname} DESTINATION bin )

ENDFOREACH()



# display some variables and write them to cache
DISPLAY_STD_VARIABLES()


# generate and install following configuration files
GENERATE_PACKAGE_CONFIGURATION_FILES( CEDConfig.cmake CEDConfigVersion.cmake CEDLibDeps.cmake )

