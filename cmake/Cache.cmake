OPTION(ENABLE_CACHE "Enable cache if available" ON)
IF(NOT ENABLE_CACHE)
    RETURN()
ENDIF()

SET(CACHE_OPTION
    "ccache"
    CACHE STRING "Compiler cache to be used")
SET(CACHE_OPTION_VALUES "ccache" "sccache")
SET_PROPERTY(CACHE CACHE_OPTION PROPERTY STRINGS ${CACHE_OPTION_VALUES})
LIST(
        FIND
        CACHE_OPTION_VALUES
        ${CACHE_OPTION}
        CACHE_OPTION_INDEX)

IF(${CACHE_OPTION_INDEX} EQUAL -1)
    MESSAGE(
            STATUS
            "Using custom compiler cache system: '${CACHE_OPTION}', explicitly supported entries are ${CACHE_OPTION_VALUES}")
ENDIF()

FIND_PROGRAM(CACHE_BINARY ${CACHE_OPTION})
IF(CACHE_BINARY)
    MESSAGE(STATUS "${CACHE_OPTION} found and enabled")
    SET(CMAKE_CXX_COMPILER_LAUNCHER ${CACHE_BINARY})
ELSE()
    MESSAGE(WARNING "${CACHE_OPTION} is enabled but was not found. Not using it")
ENDIF()
