MACRO(RUN_CONAN)
    # Download automatically, you can also just copy the conan.cmake file
    IF(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        MESSAGE(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        FILE(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.16.1/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake")
    ENDIF()

    INCLUDE(${CMAKE_BINARY_DIR}/conan.cmake)

    CONAN_ADD_REMOTE(NAME center
                     INDEX 0
                     URL https://center.conan.io
                     VERIFY_SSL True)

    CONAN_CMAKE_RUN(
            REQUIRES
            ${CONAN_EXTRA_REQUIRES}
            catch2/2.13.8
            OPTIONS
            ${CONAN_EXTRA_OPTIONS}
            BASIC_SETUP
            CMAKE_TARGETS # individual targets to link to
            BUILD
            missing)
ENDMACRO()
