########################################################
# cmake file for building CED documentation
# @author Jan Engels, DESY
########################################################



SET( DOC_SRC_DIR "${PROJECT_SOURCE_DIR}/doc" )
SET( DOC_BIN_DIR "${PROJECT_BINARY_DIR}/docbuild" )

# in-source vs out-of-source installations
IF( CMAKE_INSTALL_PREFIX STREQUAL "${PROJECT_SOURCE_DIR}" )
    # --- in-source installations ---
    SET( DOC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/doc )
ELSE()
    # --- out-of-source installations ---
    SET( DOC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/doc/${PROJECT_NAME} )
ENDIF()


# global doc target for calling doc_html and doc_latex
ADD_CUSTOM_TARGET( doc ) 


#FIND_PACKAGE( Doxygen )
#IF( DOXYGEN_FOUND )
#
#    # directories to search for documentation
#    SET( DOX_INPUT ../src)
#
#    # custom command to build documentation
#    ADD_CUSTOM_COMMAND(
#        OUTPUT  "${DOC_BIN_DIR}/html/index.html"
#        COMMAND DOX_PROJECT_NAME=${PROJECT_NAME}
#                DOX_PROJECT_NUMBER="${${PROJECT_NAME}_VERSION}"
#                DOX_OUTPUT_DIRECTORY="${DOC_BIN_DIR}"
#                DOX_INPUT="${DOX_INPUT}"
#                "${DOXYGEN_EXECUTABLE}"
#        WORKING_DIRECTORY "${DOC_SRC_DIR}"
#        COMMENT "Building API Documentation..."
#        DEPENDS Doxyfile CMakeLists.txt 
#    )
#
#    # add doc_html target
#    ADD_CUSTOM_TARGET( doc_html DEPENDS "${DOC_BIN_DIR}/html/index.html" )
#    # tell doc target to call doc_html
#    ADD_DEPENDENCIES( doc doc_html )
#
#ELSE()
#    MESSAGE( STATUS "Doxygen not found in your system!!" )
#    IF( INSTALL_DOC )
#        MESSAGE( "INSTALL_DOC set to OFF" )
#        SET( INSTALL_DOC OFF )
#    ENDIF()
#ENDIF()


FIND_PACKAGE( LATEX )
IF( PDFLATEX_COMPILER )

    # user manual
    ADD_CUSTOM_COMMAND(
        OUTPUT  "${DOC_BIN_DIR}/manual"
        # first we copy directory from source tree to binary tree to
        # keep the source tree unchanged
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${DOC_SRC_DIR}/manual"
            "${DOC_BIN_DIR}/manual"
    )

    SET_DIRECTORY_PROPERTIES( PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
        "${DOC_BIN_DIR}/manual"
    )

    ADD_CUSTOM_COMMAND(
        OUTPUT  "${DOC_INSTALL_PREFIX}/manual.pdf"
        COMMAND "${PDFLATEX_COMPILER}" ARGS -interaction=batchmode manual.tex
        COMMAND "${PDFLATEX_COMPILER}" ARGS -interaction=batchmode manual.tex
        # copy files to doc directory
        COMMAND "${CMAKE_COMMAND}" -E copy "manual.pdf" "${DOC_INSTALL_PREFIX}/manual.pdf"
        DEPENDS "${DOC_BIN_DIR}/manual"
        WORKING_DIRECTORY "${DOC_BIN_DIR}/manual"
        COMMENT "Building Documentation - Users Manual..."
    )

    ADD_CUSTOM_TARGET( doc_manual DEPENDS "${DOC_INSTALL_PREFIX}/manual.pdf" )

    ADD_DEPENDENCIES( doc doc_manual )

    # uninstall rule to delete manual.pdf
    ADD_CUSTOM_TARGET( uninstall_doc
        #COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_INSTALL_PREFIX}/doc/html"
        COMMAND "${CMAKE_COMMAND}" -E remove "${DOC_INSTALL_PREFIX}/manual.pdf"
    )
    ADD_DEPENDENCIES( uninstall uninstall_doc )

ELSE()
    MESSAGE( STATUS "pdflatex not found in your system!!" )
    IF( INSTALL_DOC )
        MESSAGE( "INSTALL_DOC set to OFF" )
        SET( INSTALL_DOC OFF )
    ENDIF()
ENDIF()


IF( INSTALL_DOC )
    # make sure doxygen is executed (make doc) before make install
    INSTALL( CODE "EXECUTE_PROCESS( COMMAND ${CMAKE_BUILD_TOOL} doc)" )

    # install html documentation
    #INSTALL_DIRECTORY( "${DOC_BIN_DIR}/html" DESTINATION ${DOC_INSTALL_PREFIX} )
ENDIF()

