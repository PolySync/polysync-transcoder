# Some of the plugins (dump and datamodel) do not appear here as separate
# libraries, because they are hard linked into the transcode command line tool.

#add_library( csv MODULE csv.cpp )
#set_target_properties( csv PROPERTIES PREFIX "transcode." )

add_library( plog_encode MODULE plog_encode.cpp )
target_link_libraries ( plog_encode plog dl )
set_target_properties( plog_encode PROPERTIES PREFIX "transcode." )

add_library( matlab MODULE matlab.cpp )
target_link_libraries ( matlab plog dl )
set_target_properties( matlab PROPERTIES PREFIX "transcode." )

set ( HDF5_FOUND FALES )
if ( ${HDF5_FOUND} )
    include_directories( ${HDF5_INCLUDE_DIRS} )
    add_library( hdf5 MODULE hdf5.cpp )
    target_link_libraries ( hdf5 ${HDF5_LIBRARIES} )
    set_target_properties( hdf5 PROPERTIES PREFIX "transcode." )
endif()

set ( SCIDB_FOUND FALSE )
if ( ${SCIDB_FOUND} )
    add_library( scidb MODULE scidb.cpp )
    target_link_libraries ( scidb ${SCIDB_LIBRARIES} )
    set_target_properties( scidb PROPERTIES PREFIX "transcode." )
endif()


