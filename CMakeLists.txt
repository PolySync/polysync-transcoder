cmake_minimum_required ( VERSION 2.8 )
project ( polysync-transcode )

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -ferror-limit=5")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

if ( !${JENKINS} )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

find_package(Threads)
find_package(Boost "1.61" COMPONENTS log log_setup system filesystem 
             program_options thread regex iostreams )
find_package(PythonLibs)
find_package(HDF5 COMPONENTS HL)

include_directories( . "deps" "deps/mettle/include" "deps/eggs/include" )
add_subdirectory( lib )
add_subdirectory( cmdline )
add_subdirectory( plugin )
add_subdirectory( deps/mettle )
add_subdirectory( test )

enable_testing()
foreach ( component IN ITEMS decode_hana tree size description detector 
          encode decode )
    add_test(NAME ${component} COMMAND test_${component} )
endforeach( component )
