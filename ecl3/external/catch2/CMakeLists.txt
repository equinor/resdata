cmake_minimum_required(VERSION 3.5)
project(catch2 VERSION 2.7.2)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif ()

# as long as ecl2 is still the root project, and uses catch2, add as library as
# ecl3-catch2. This way, ecl3 can still build independently from ecl2
# TODO: use a single catch2 instance
add_library(ecl3-catch2 INTERFACE)
add_library(ecl3::catch2 ALIAS ecl3-catch2)
target_include_directories(ecl3-catch2
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
