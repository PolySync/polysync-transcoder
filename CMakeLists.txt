cmake_minimum_required ( VERSION 2.8 )
project ( polysync-transcode )

if ( EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake" )
    # Inside of PolySync development team, use conan to sync the dependencies
    include( ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake )
    conan_basic_setup()
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -ferror-limit=10")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

if ( !${JENKINS} )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

find_package(Threads)
find_package(Boost COMPONENTS log log_setup system filesystem program_options iostreams thread )

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
include_directories( . "${CMAKE_SOURCE_DIR}/vendor" "${CMAKE_SOURCE_DIR}/vendor/eggs/include" ${Boost_INCLUDE_DIRS} )
add_subdirectory( lib )
add_subdirectory( cmdline )
add_subdirectory( plugin )
add_subdirectory( test )

enable_testing()
foreach ( component IN ITEMS decode_hana tree size description detector 
          encode decode )
    add_test(NAME ${component} COMMAND test_${component} )
endforeach( component )

install( DIRECTORY polysync/ 
         DESTINATION include/polysync 
         FILES_MATCHING PATTERN "*.hpp" 
         PERMISSIONS WORLD_READ )
     
install( DIRECTORY share/ 
         DESTINATION share/polysync-transcoder
         FILES_MATCHING PATTERN "*.toml" 
         PERMISSIONS WORLD_READ )

add_custom_target( run-unit-tests COMMAND mettle )
