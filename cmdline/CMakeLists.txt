add_executable ( polysync-transcode transcode.cpp toml.cpp encode.cpp filter.cpp
    ../plugin/encode/dump.cpp ../plugin/encode/list.cpp ../plugin/filter/slice.cpp 
    ../plugin/filter/typefilter.cpp )
target_link_libraries ( polysync-transcode transcode ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} dl )
install( TARGETS polysync-transcode 
         DESTINATION bin 
         PERMISSIONS WORLD_EXECUTE )


