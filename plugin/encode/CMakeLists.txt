# Some of the plugins (dump and datamodel) do not appear here as separate
# libraries, because they are hard linked into the transcode command line tool.

add_library( plog MODULE plog.cpp )
target_link_libraries ( plog transcode dl )
set_target_properties( plog PROPERTIES PREFIX "encode." )
install( TARGETS plog DESTINATION share/polysync-transcoder/plugin )

