add_library ( transcode SHARED logging.cpp detector.cpp exception.cpp 
              descriptor/toml.cpp descriptor/catalog.cpp descriptor/lex.cpp 
              descriptor/print.cpp descriptor/formatter.cpp
              decoder.cpp encoder.cpp
              )
add_library ( plog_transcode SHARED plog/core.cpp plog/magic.cpp )
target_link_libraries( transcode ${Boost_LIBRARIES} )
install( TARGETS transcode plog_transcode DESTINATION lib/polysync-transcoder )
