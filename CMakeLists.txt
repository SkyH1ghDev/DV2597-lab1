cmake_minimum_required(VERSION 3.23)

project(DV2597-lab1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_COMPILER g++)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(gauss_sequential)
add_executable(gauss_parallel)
add_executable(qsort_sequential)
add_executable(qsort_parallel)

# GAUSS
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

# QSORT
target_link_libraries(qsort_parallel PRIVATE Threads::Threads)

target_sources(
    qsort_sequential
    PRIVATE
        qsortseq.c
)
target_sources(
    qsort_parallel
    PRIVATE
        qsortpar.cpp
)