add_library ( transcode SHARED logging.cpp detector.cpp exception.cpp 
              descriptor/toml.cpp descriptor/catalog.cpp descriptor/lex.cpp 
              descriptor/print.cpp descriptor/formatter.cpp
              plog/core.cpp plog/decoder.cpp plog/encoder.cpp plog/iterator.cpp plog/magic.cpp )
target_link_libraries ( transcode ${Boost_LIBRARIES} )
install( TARGETS transcode DESTINATION lib )

