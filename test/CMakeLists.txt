foreach ( component IN ITEMS decode_hana tree size description detector encode decode )
    add_executable ( test_${component} ${component}.cpp )
    target_link_libraries ( test_${component} plog mettle ${Boost_LIBRARIES} )
endforeach( component )
