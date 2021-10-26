cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 17)
project(scclient-cpp VERSION 1.0)

add_definitions(-std=c++17 -Wno-deprecated -Wno-nullability-completeness -Wno-deprecated-declarations)

set(SOURCES
    # Lib cpp file
    src/scclient.cpp
)

find_package(libwebsockets CONFIG REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${LWS_CMAKE_DIR})
include(LwsCheckRequirements)

add_executable(example_client_simple examples/example_client_simple.cpp ${SOURCES})
add_executable(example_client_classed examples/example_client_classed.cpp ${SOURCES})


target_include_directories(example_client_simple PRIVATE HEADER_FILE_ONLY "${PROJECT_SOURCE_DIR}/include")
target_include_directories(example_client_simple PRIVATE HEADER_FILE_ONLY "/usr/local/include/")
target_link_libraries(example_client_simple PUBLIC -L/usr/local/lib -lwebsockets -ljson-c)

target_include_directories(example_client_classed PRIVATE HEADER_FILE_ONLY "${PROJECT_SOURCE_DIR}/include")
target_include_directories(example_client_classed PRIVATE HEADER_FILE_ONLY "/usr/local/include/")
target_link_libraries(example_client_classed PUBLIC -L/usr/local/lib -lwebsockets -ljson-c)