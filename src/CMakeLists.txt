cmake_minimum_required(VERSION 3.15)
project(asabru_parsers)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings
if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wall)
endif()

# Debug preprocessor definition
add_compile_definitions(DEBUG)

# add dependencies
include(../cmake/CPM.cmake)

if (ASABRU_COMMONS_BUILD STREQUAL "GIT_TAG")
    CPMAddPackage(
            NAME asabru_commons
            GITHUB_REPOSITORY ChistaDATA/asabru-commons
            GIT_TAG ${ASABRU_COMMONS_GIT_TAG}
    )
elseif (ASABRU_COMMONS_BUILD STREQUAL "LOCAL_DIR")
    # For development purpose
    CPMAddPackage(
            NAME asabru_commons
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../asabru-commons/src
    )
else()
    CPMAddPackage(
            NAME asabru_commons
            GITHUB_REPOSITORY ChistaDATA/asabru-commons
            VERSION 0.0.2
    )
endif()

# Set source directory
set(SOURCE_DIRECTORY ".")

# Collect all source files
file(GLOB_RECURSE SOURCE_FILES "${SOURCE_DIRECTORY}/*.cpp")

# Add build directory
set(BUILD_DIRECTORY "build")

# Set library directory
set(LIB_DIRECTORY "lib")

# Create a library target
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Set output directory for libraries
set_target_properties(${PROJECT_NAME}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${LIB_DIRECTORY}
)

target_link_libraries(
        ${PROJECT_NAME}
        asabru_commons
)