#
# This function will prevent in-source builds
FUNCTION(ASSUREOUTOFSOURCEBUILDS)
    # make sure the user doesn't play dirty with symlinks
    GET_FILENAME_COMPONENT(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
    GET_FILENAME_COMPONENT(bindir "${CMAKE_BINARY_DIR}" REALPATH)

    # disallow in-source builds
    IF("${srcdir}" STREQUAL "${bindir}")
        MESSAGE("######################################################")
        MESSAGE("Warning: in-source builds are disabled")
        MESSAGE("Please create a separate build directory and run cmake from there")
        MESSAGE("######################################################")
        MESSAGE(FATAL_ERROR "Quitting configuration")
    ENDIF()
ENDFUNCTION()

ASSUREOUTOFSOURCEBUILDS()
