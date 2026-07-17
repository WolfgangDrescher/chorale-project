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

install(TARGETS chorale-search RUNTIME DESTINATION bin)
