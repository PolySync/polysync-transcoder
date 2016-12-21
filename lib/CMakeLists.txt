add_library ( transcode SHARED logging.cpp descriptor.cpp detector.cpp 
              exception.cpp plog/core.cpp plog/decoder.cpp plog/encoder.cpp )
target_link_libraries ( transcode ${Boost_LIBRARIES} )

