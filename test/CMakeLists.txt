foreach ( component IN ITEMS decode_hana tree size description detector encode decode )
    add_executable ( ${component} ${component}.cpp )
    target_link_libraries ( ${component} plog mettle ${Boost_LIBRARIES} )
endforeach( component )
