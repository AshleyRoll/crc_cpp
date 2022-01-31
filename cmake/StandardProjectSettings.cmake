# Set a default build type if none was specified
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    MESSAGE(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    SET(CMAKE_BUILD_TYPE
        RelWithDebInfo
        CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui, ccmake
    SET_PROPERTY(
            CACHE CMAKE_BUILD_TYPE
            PROPERTY STRINGS
            "Debug"
            "Release"
            "MinSizeRel"
            "RelWithDebInfo")
ENDIF()

# Generate compile_commands.json to make it easier to work with clang based tools
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

OPTION(ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" OFF)

IF(ENABLE_IPO)
    INCLUDE(CheckIPOSupported)
    CHECK_IPO_SUPPORTED(
            RESULT
            result
            OUTPUT
            output)
    IF(result)
        SET(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    ELSE()
        MESSAGE(SEND_ERROR "IPO is not supported: ${output}")
    ENDIF()
ENDIF()
IF(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    ADD_COMPILE_OPTIONS(-fcolor-diagnostics)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    ADD_COMPILE_OPTIONS(-fdiagnostics-color=always)
ELSE()
    MESSAGE(STATUS "No colored compiler diagnostic set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
ENDIF()

