add_executable ( polysync-transcode transcode.cpp ../plugin/dump.cpp ../plugin/list.cpp )
target_link_libraries ( polysync-transcode transcode ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} dl )

