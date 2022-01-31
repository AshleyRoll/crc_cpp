OPTION(ENABLE_CPPCHECK "Enable static analysis with cppcheck" OFF)
OPTION(ENABLE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)
OPTION(ENABLE_INCLUDE_WHAT_YOU_USE "Enable static analysis with include-what-you-use" OFF)

IF(ENABLE_CPPCHECK)
    FIND_PROGRAM(CPPCHECK cppcheck)
    IF(CPPCHECK)
        SET(CMAKE_CXX_CPPCHECK
            ${CPPCHECK}
            --suppress=missingInclude
            --enable=all
            --inline-suppr
            --inconclusive
            -i
            ${CMAKE_SOURCE_DIR}/imgui/lib)
    ELSE()
        MESSAGE(SEND_ERROR "cppcheck requested but executable not found")
    ENDIF()
ENDIF()

IF(ENABLE_CLANG_TIDY)
    FIND_PROGRAM(CLANGTIDY clang-tidy)
    IF(CLANGTIDY)
        SET(CMAKE_CXX_CLANG_TIDY ${CLANGTIDY} -extra-arg=-Wno-unknown-warning-option)
    ELSE()
        MESSAGE(SEND_ERROR "clang-tidy requested but executable not found")
    ENDIF()
ENDIF()

IF(ENABLE_INCLUDE_WHAT_YOU_USE)
    FIND_PROGRAM(INCLUDE_WHAT_YOU_USE include-what-you-use)
    IF(INCLUDE_WHAT_YOU_USE)
        SET(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE})
    ELSE()
        MESSAGE(SEND_ERROR "include-what-you-use requested but executable not found")
    ENDIF()
ENDIF()
