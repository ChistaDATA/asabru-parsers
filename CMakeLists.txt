cmake_minimum_required(VERSION 3.8)
project(asabru_parsers)

include_directories(../asabru-commons/src)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings
if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wall)
endif()

# Set source directory
set(SOURCE_DIRECTORY "src")

# Collect all source files
file(GLOB_RECURSE SOURCE_FILES "${SOURCE_DIRECTORY}/*.cpp")

# Add build directory
set(BUILD_DIRECTORY "build")

# Set library directory
set(LIB_DIRECTORY "lib")

# Create a library target
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

# Set output directory for libraries
set_target_properties(${PROJECT_NAME} 
  PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${LIB_DIRECTORY}
)

target_link_libraries(
    ${PROJECT_NAME} 
        ${CMAKE_SOURCE_DIR}/lib/asabru-commons/lib/asabru-commons.a
)



