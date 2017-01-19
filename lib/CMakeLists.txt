add_library ( transcode SHARED logging.cpp exception.cpp 
              descriptor/toml.cpp descriptor/catalog.cpp descriptor/lex.cpp 
              descriptor/print.cpp descriptor/formatter.cpp
              decoder.cpp 
              detector/catalog.cpp detector/search.cpp
              )
add_library ( plog_transcode SHARED plog/core.cpp plog/encoder.cpp plog/magic.cpp )
target_link_libraries( transcode ${Boost_LIBRARIES} )
install( TARGETS transcode plog_transcode DESTINATION lib/polysync-transcoder )
