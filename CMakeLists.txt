cmake_minimum_required(VERSION 3.23)

project(DV2597-lab1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_COMPILER g++)

add_executable(gauss_sequential)
add_executable(gauss_parallel)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(gauss_parallel PRIVATE Threads::Threads)

target_sources(
    gauss_sequential
    PRIVATE
        gaussianseq.cpp
)

target_sources(
    gauss_parallel
    PRIVATE
        gaussianpar.cpp
)