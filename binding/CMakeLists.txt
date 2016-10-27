include_directories( ${PYTHON_INCLUDE_DIRS} )
add_library ( polysync SHARED python.cpp )
target_link_libraries ( polysync ${PYTHON_LIBRARIES} ${Boost_LIBRARIES} )
set_target_properties( polysync PROPERTIES PREFIX "" )

