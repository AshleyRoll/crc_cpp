# Automatically enable catch2 to generate ctest targets
IF(CONAN_CATCH2_ROOT_DEBUG)
    INCLUDE(${CONAN_CATCH2_ROOT_DEBUG}/lib/cmake/Catch2/Catch.cmake)
ELSE()
    INCLUDE(${CONAN_CATCH2_ROOT}/lib/cmake/Catch2/Catch.cmake)
ENDIF()

ADD_EXECUTABLE(tests test.cpp)
TARGET_LINK_LIBRARIES(tests PRIVATE project_warnings project_options CONAN_PKG::catch2)
TARGET_INCLUDE_DIRECTORIES(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# automatically discover tests that are defined in catch based test files you can modify the unittests. Set TEST_PREFIX
# to whatever you want, or use different for different binaries
CATCH_DISCOVER_TESTS(
        tests
        TEST_PREFIX
        "unittests."
        REPORTER
        JUnit
        OUTPUT_DIR
        .
        OUTPUT_PREFIX
        "unittests."
        OUTPUT_SUFFIX
        .xml)
