add_executable ( transcode transcode.cpp core.cpp decoder.cpp logging.cpp description.cpp ../plugin/dump.cpp ../plugin/datamodel.cpp )
add_definitions ( -DTRANSCODE_PLUGIN_PATH="transcode" ) # FIXME; relative to PSYNC_HOME?
target_link_libraries ( transcode ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} dl )

