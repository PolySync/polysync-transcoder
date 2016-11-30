add_library ( plog SHARED core.cpp logging.cpp decoder.cpp encoder.cpp description.cpp 
              detector.cpp exception.cpp )
add_executable ( transcode transcode.cpp ../plugin/dump.cpp ../plugin/model.cpp )
add_definitions ( -DTRANSCODE_PLUGIN_PATH="transcode" ) # FIXME; relative to PSYNC_HOME?
target_link_libraries ( transcode plog ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} dl )

