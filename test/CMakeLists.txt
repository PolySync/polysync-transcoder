foreach ( component IN ITEMS decode tree size description detector )
    add_executable ( test_${component} ${component}.cpp )
    target_link_libraries ( test_${component} plog mettle ${Boost_LIBRARIES} )
endforeach( component )
