add_executable ( transcode transcode.cpp core.cpp dynamic_reader.cpp logging.cpp description.cpp legacy.cpp query.cpp dump.cpp plog.cpp csv.cpp )
add_definitions ( -DTRANSCODE_PLUGIN_PATH="transcode" ) # FIXME; relative to PSYNC_HOME?
target_link_libraries ( transcode ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} dl )

if ( ${HDF5_FOUND} )
    include_directories( ${HDF5_INCLUDE_DIRS} )
    add_library( hdf5 MODULE hdf5.cpp )
    target_link_libraries ( hdf5 ${HDF5_LIBRARIES} )
    set_target_properties( hdf5 PROPERTIES PREFIX "transcode." )
endif()

set ( SCIDB_FOUND TRUE )
if ( ${SCIDB_FOUND} )
    add_library( scidb MODULE scidb.cpp )
    target_link_libraries ( scidb ${SCIDB_LIBRARIES} )
    set_target_properties( scidb PROPERTIES PREFIX "transcode." )
endif()

set ( MATLAB_FOUND TRUE )
if ( ${SCIDB_FOUND} )
    add_library( matlab MODULE matlab.cpp )
    target_link_libraries ( matlab ${MATLAB_LIBRARIES} )
    set_target_properties( matlab PROPERTIES PREFIX "transcode." )
endif()


include_directories( ${PYTHON_INCLUDE_DIRS} )
add_library ( polysync SHARED python.cpp )
target_link_libraries ( polysync ${PYTHON_LIBRARIES} ${Boost_LIBRARIES} )
set_target_properties( polysync PROPERTIES PREFIX "" )

