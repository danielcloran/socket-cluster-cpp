cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 17)
project(scclient-cpp VERSION 1.0)

add_definitions(-std=c++17 -Wno-deprecated -Wno-nullability-completeness -Wno-deprecated-declarations)

set(SOURCES
    src/SCClient.cpp
)

find_package(Boost 1.76.0 REQUIRED) # header only libraries must not be added here

add_executable(example_client_simple examples/example_client_simple.cpp ${SOURCES})
add_executable(example_client_classed examples/example_client_classed.cpp ${SOURCES})

target_include_directories(example_client_simple PRIVATE HEADER_FILE_ONLY "${PROJECT_SOURCE_DIR}/include")
target_include_directories(example_client_simple PRIVATE HEADER_FILE_ONLY "/usr/local/include/")
target_include_directories(example_client_simple PUBLIC "${Boost_INCLUDE_DIRS}")
target_link_libraries(example_client_simple PUBLIC -L/usr/local/lib -ljson-c)

target_include_directories(example_client_classed PRIVATE HEADER_FILE_ONLY "${PROJECT_SOURCE_DIR}/include")
target_include_directories(example_client_classed PRIVATE HEADER_FILE_ONLY "/usr/local/include/")
target_include_directories(example_client_classed PUBLIC "${Boost_INCLUDE_DIRS}")
target_link_libraries(example_client_classed PUBLIC -L/usr/local/lib -ljson-c ${Boost_LIBRARIES})