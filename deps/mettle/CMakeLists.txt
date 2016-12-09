# Custom CMakeLists.txt for distributing with polysync-transcode.  Mettle ships
# with a build system dependency bfg9000 which in turn depends on Python.  I
# don't want this dependency.

set ( SRCDIR src/libmettle )
add_library ( mettle SHARED 
    ${SRCDIR}/cmd_line.cpp ${SRCDIR}/driver.cpp ${SRCDIR}/filters.cpp 
    ${SRCDIR}/log/brief.cpp ${SRCDIR}/log/summary.cpp ${SRCDIR}/log/verbose.cpp 
    ${SRCDIR}/log/posix/term.cpp
    ${SRCDIR}/posix/subprocess.cpp ${SRCDIR}/posix/subprocess_test_runner.cpp
    )

