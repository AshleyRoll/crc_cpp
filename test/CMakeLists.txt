# Automatically enable catch2 to generate ctest targets
IF(CONAN_CATCH2_ROOT_DEBUG)
    INCLUDE(${CONAN_CATCH2_ROOT_DEBUG}/lib/cmake/Catch2/Catch.cmake)
ELSE()
    INCLUDE(${CONAN_CATCH2_ROOT}/lib/cmake/Catch2/Catch.cmake)
ENDIF()

ADD_LIBRARY(catch_main STATIC catch_main.cpp)
TARGET_LINK_LIBRARIES(catch_main PUBLIC CONAN_PKG::catch2)
TARGET_LINK_LIBRARIES(catch_main PRIVATE project_options)

ADD_EXECUTABLE(tests test.cpp)
TARGET_LINK_LIBRARIES(tests PRIVATE project_warnings project_options catch_main)
TARGET_INCLUDE_DIRECTORIES(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# automatically discover tests that are defined in catch based test files you can modify the unittests. Set TEST_PREFIX
# to whatever you want, or use different for different binaries
CATCH_DISCOVER_TESTS(
        tests
        TEST_PREFIX
        "unittests."
        REPORTER
        xml
        OUTPUT_DIR
        .
        OUTPUT_PREFIX
        "unittests."
        OUTPUT_SUFFIX
        .xml)
