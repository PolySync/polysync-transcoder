include_directories ( /home/jgalbraith/cucumber-cpp/include )
add_executable ( plog_steps plog.cpp )
set (CUKE_LIBRARIES /home/jgalbraith/cucumber-cpp/build/src/libcucumber-cpp.a)
target_link_libraries ( plog_steps plog mettle ${CUKE_LIBRARIES} ${Boost_LIBRARIES} )
