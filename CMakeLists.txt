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
    src/utils/app_context.cpp
    src/utils/command_handler.cpp
    src/generator/xsd_schema.cpp
    src/generator/xsd_parser.cpp
    src/generator/data_generator.cpp
    src/generator/xml_generator.cpp
    src/validator/xml_validator.cpp
)

# Create executable
add_executable(expocli ${SOURCES} ${pugixml_SOURCE_DIR}/src/pugixml.cpp)

# Find and link readline library
find_library(READLINE_LIBRARY NAMES readline)
if(READLINE_LIBRARY)
    target_link_libraries(expocli ${READLINE_LIBRARY})
    message(STATUS "Found readline library: ${READLINE_LIBRARY}")
else()
    message(WARNING "readline library not found - command history will not work")
endif()

# Install target
install(TARGETS expocli DESTINATION bin)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
