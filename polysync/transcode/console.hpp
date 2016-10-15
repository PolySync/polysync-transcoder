#pragma once

// Define console output styles.  This should be easy to extend to arbitrary
// formatting schemes if anyone ever cares to customize the look.  The nocolor
// style is important because the escape codes end up uglifying output devices
// that are not ansi-aware (like debugger displays). 

namespace polysync { namespace console {

struct codes {
    const char *normal;
    const char *black;
    const char *red;
    const char *green;
    const char *yellow;
    const char *blue;
    const char *magenta;
    const char *cyan;
    const char *white;
    const char *bold;
};

struct color : codes {
    color() : codes { "\x1B[0m", "\x1B[30m", "\x1B[31m", "\x1B[32m", "\x1B[33m", 
        "\x1B[34m", "\x1B[35m", "\x1B[36m", "\x1B[37m", "\x1B[1m" } {};
};

struct nocolor : codes {
    nocolor() : codes { "", "", "", "", "", "", "", "", "", "" } {};
};

extern codes format;

}} // namespace polysync::console

