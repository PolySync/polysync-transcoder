# Custom CMakeLists.txt for distributing with polysync-transcode.  Mettle ships
# with a build system dependency bfg9000 which in turn depends on Python.  I
# don't want this dependency, and would rather have a simpler build without any
# additional dependencies.

set ( LIBDIR src/libmettle )
set ( SRCDIR src/mettle )
add_library ( libmettle SHARED 
	${LIBDIR}/cmd_line.cpp ${LIBDIR}/driver.cpp ${LIBDIR}/filters.cpp 
	${LIBDIR}/log/brief.cpp ${LIBDIR}/log/summary.cpp ${LIBDIR}/log/verbose.cpp 
	${LIBDIR}/log/posix/term.cpp
	${LIBDIR}/posix/subprocess.cpp ${LIBDIR}/posix/subprocess_test_runner.cpp
    )
set_target_properties ( libmettle PROPERTIES PREFIX "" )
add_executable ( mettle ${SRCDIR}/mettle.cpp ${SRCDIR}/run_test_files.cpp
                 ${SRCDIR}/test_file.cpp ${SRCDIR}/posix/run_test_file.cpp )
target_link_libraries ( mettle libmettle ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
