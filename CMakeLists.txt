cmake_minimum_required ( VERSION 2.8 )
project ( gcpp_api )

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics -std=c++14 -ferror-limit=5")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

find_package(Threads)
find_package(Boost COMPONENTS log log_setup system filesystem program_options thread)
find_package(PythonLibs)
find_package(HDF5 COMPONENTS HL)

include_directories( . )
add_subdirectory( transcode )
add_subdirectory( test )

