foreach ( component IN ITEMS decode_hana tree size descriptor detector 
          encode decode print )
    add_executable ( ${component} ${component}.cpp )
    target_link_libraries ( ${component} transcode plog_transcode mettle 
	${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
    set_target_properties ( ${component} PROPERTIES PREFIX "ut." )
endforeach( component )
