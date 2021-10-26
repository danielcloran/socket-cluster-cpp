cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 17)
project(scclient-cpp VERSION 1.0)

add_definitions(-std=c++17 -Wno-deprecated -Wno-nullability-completeness -Wno-deprecated-declarations)

set(SOURCES
    # Lib cpp file
    src/scclient.cpp
    # Example
    examples/example_client.cpp
)

find_package(libwebsockets CONFIG REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${LWS_CMAKE_DIR})
include(LwsCheckRequirements)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE HEADER_FILE_ONLY "${PROJECT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE HEADER_FILE_ONLY "/usr/local/include/")

target_link_libraries(${PROJECT_NAME} PUBLIC -L/usr/local/lib -lwebsockets -ljson-c)