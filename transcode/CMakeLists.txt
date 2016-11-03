add_library ( plog SHARED core.cpp decoder.cpp description.cpp detector.cpp logging.cpp )
add_executable ( transcode transcode.cpp ../plugin/dump.cpp ../plugin/datamodel.cpp )
add_definitions ( -DTRANSCODE_PLUGIN_PATH="transcode" ) # FIXME; relative to PSYNC_HOME?
target_link_libraries ( transcode plog ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} dl )

