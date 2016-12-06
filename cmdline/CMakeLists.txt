add_executable ( polysync-transcode transcode.cpp toml.cpp plugin.cpp 
    ../plugin/encode/dump.cpp ../plugin/encode/list.cpp ../plugin/filter/slice.cpp )
target_link_libraries ( polysync-transcode transcode ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} dl )

