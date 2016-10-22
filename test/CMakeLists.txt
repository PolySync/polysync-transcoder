foreach ( component IN ITEMS variant size )
    add_executable ( test_${component} ${component}.cpp )
    target_link_libraries ( test_${component} mettle ${Boost_LIBRARIES} )
endforeach( component )
