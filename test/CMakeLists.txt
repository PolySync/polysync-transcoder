foreach ( component IN ITEMS decode_hana tree size description detector 
          encode decode print )
    add_executable ( ${component} ${component}.cpp )
    target_link_libraries ( ${component} transcode mettle ${Boost_LIBRARIES} )
endforeach( component )
