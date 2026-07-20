cmake_minimum_required(VERSION 3.16)
project(chorale-search CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/humlib/min/humlib.h")
    message(FATAL_ERROR "external/humlib is missing. Run: ./scripts/fetch-deps.sh")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include/nlohmann/json.hpp")
    message(FATAL_ERROR "external/json is missing. Run: ./scripts/fetch-deps.sh")
endif()

add_library(humlib STATIC
    external/humlib/min/humlib.cpp
    external/humlib/min/pugixml.cpp
)
target_include_directories(humlib SYSTEM PUBLIC external/humlib/min)
target_compile_options(humlib PRIVATE -w)

add_library(chorale_core STATIC
    src/HumdrumChorale.cpp
    src/VoiceMap.cpp
    src/AttributeMatcher.cpp
    src/CorpusSearch.cpp
    src/JsonIO.cpp
)
target_include_directories(chorale_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include
)
target_compile_options(chorale_core PRIVATE -Wall -Wextra)
target_link_libraries(chorale_core PUBLIC humlib)

add_executable(chorale-search cli/chorale-search.cpp)
target_link_libraries(chorale-search PRIVATE chorale_core)

add_executable(add-modulations cli/add-modulations.cpp)
target_include_directories(add-modulations PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include)
target_compile_options(add-modulations PRIVATE -Wall -Wextra)
target_link_libraries(add-modulations PRIVATE humlib)

install(TARGETS chorale-search RUNTIME DESTINATION bin)

option(BUILD_TESTS "Build the minitest test binaries in tests/" ON)

if(BUILD_TESTS)
    enable_testing()

    add_executable(test_voicemap tests/test_voicemap.cpp src/VoiceMap.cpp)
    target_include_directories(test_voicemap PRIVATE src tests)
    add_test(NAME voicemap COMMAND test_voicemap)

    add_executable(test_jsonio tests/test_jsonio.cpp src/JsonIO.cpp)
    target_include_directories(test_jsonio PRIVATE src tests external/json/single_include)
    add_test(NAME jsonio COMMAND test_jsonio)

    add_executable(test_humdrumchorale tests/test_humdrumchorale.cpp)
    target_include_directories(test_humdrumchorale PRIVATE tests)
    target_link_libraries(test_humdrumchorale PRIVATE chorale_core)
    add_test(NAME humdrumchorale COMMAND test_humdrumchorale)

    add_executable(test_corpussearch tests/test_corpussearch.cpp)
    target_include_directories(test_corpussearch PRIVATE tests)
    target_link_libraries(test_corpussearch PRIVATE chorale_core)
    add_test(NAME corpussearch COMMAND test_corpussearch)

    add_executable(test_attributematcher tests/test_attributematcher.cpp)
    target_include_directories(test_attributematcher PRIVATE tests)
    target_link_libraries(test_attributematcher PRIVATE chorale_core)
    add_test(NAME attributematcher COMMAND test_attributematcher)

    add_executable(test_cli tests/test_cli.cpp)
    target_include_directories(test_cli PRIVATE tests ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include)
    target_compile_definitions(test_cli PRIVATE CHORALE_SEARCH_BINARY="$<TARGET_FILE:chorale-search>")
    add_dependencies(test_cli chorale-search)
    add_test(NAME cli COMMAND test_cli)
endif()
