cmake_minimum_required(VERSION 3.15)
project(XMLQueryCLI VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Download pugixml using FetchContent
include(FetchContent)
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        v1.14
)
FetchContent_MakeAvailable(pugixml)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${pugixml_SOURCE_DIR}/src
)

# Source files
set(SOURCES
    src/main.cpp
    src/parser/lexer.cpp
    src/parser/parser.cpp
    src/executor/query_executor.cpp
    src/executor/xml_navigator.cpp
    src/utils/xml_loader.cpp
    src/utils/result_formatter.cpp
)

# Create executable
add_executable(xmlquery ${SOURCES} ${pugixml_SOURCE_DIR}/src/pugixml.cpp)

# Link libraries (pugixml is header-only style, included directly)

# Install target
install(TARGETS xmlquery DESTINATION bin)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
