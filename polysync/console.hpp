#pragma once

// Define console output styles.  This should be easy to extend to arbitrary
// formatting schemes if anyone ever cares to customize the look.  The nocolor
// style is important because the escape codes end up uglifying output devices
// that are not ansi-aware, like debugger displays, and a GUI tool would
// certainly want to control the look by itself. 

#include <string>
#include <vector>
#include <iostream>

namespace polysync { namespace console {

// Standard ANSI terminal escape codes
struct escape {
    static constexpr const char *normal = "\x1B[0m";
    static constexpr const char *bold = "\x1B[1m";
    static constexpr const char *black = "\x1B[3m";
    static constexpr const char *red = "\x1B[31m";
    static constexpr const char *green = "\x1B[32m";
    static constexpr const char *yellow = "\x1B[33m";
    static constexpr const char *blue = "\x1B[34m";
    static constexpr const char *magenta = "\x1B[35m";
    static constexpr const char *cyan = "\x1B[36m";
    static constexpr const char *white = "\x1B[37m";
};

// Styling aliases used through the transcoder
struct style {
    const char *normal {};
    const char *bold {};
    const char *tpname {}; // type name
    const char *fieldname {};
    const char *value {};
    const char *channel {};
    const char *note {};

    const char *error {};
    const char *warn {};
    const char *info {};
    const char *verbose {};
    const char *debug1 {};
    const char *debug2 {};
};

// Default colorful style
struct color : style {
    color() : style { 
        // Explicit terms
        escape::normal, escape::bold, 
            
        // Messages
        escape::green, escape::cyan, escape::blue, escape::green, escape::white,

        // Logging severity levels
        escape::red, escape::magenta, escape::yellow, escape::green, escape::white, escape::white 
        }
    {}
};

struct markdown : style {
    markdown() : style {
        // Explicit terms
        "", "",
        // Messages
        "", "", "", "", "",
        // Logging severity levels
        "", "", "", "", "", ""
    } {}
};

// Default non-colorful style
struct nocolor : style {};

extern style format;

} // namespace console

namespace formatter {

using polysync::console::escape;

struct interface {
    virtual void begin_block( const std::string& ) const = 0;
    virtual void end_block() const = 0;
    virtual void item( const std::string&, const std::string&, const std::string& = "" ) const = 0;
    virtual void begin_ordered() const = 0;
    virtual void end_ordered() const = 0;

    std::ostream& os;
    bool show_type { true };

    interface(std::ostream& os) : os(os) { }
};

struct console : interface {
    using interface::interface;

    std::string indent { "    " };

    void begin_block(const std::string& name) const override { 
        os << tab.back() << escape::cyan << escape::bold << name << " {" 
           << escape::normal << std::endl;
        tab.push_back(tab.back() + indent);
    }

    void end_block() const override {
        tab.pop_back();
        os << tab.back() << escape::cyan << escape::bold << "}" << escape::normal << std::endl;
    }

    void item(const std::string& name, const std::string& value, const std::string& type) const override {
        os << tab.back() << escape::green << name << ": " << escape::normal 
           << value << escape::normal;
        if (!type.empty() && show_type)
            os << escape::yellow << " (" << type << ")" << escape::normal;
        os << std::endl;
    }

    void begin_ordered() const override { }

    void end_ordered() const override { }

    mutable std::vector<std::string> tab { "" };
};

struct markdown : interface {
    using interface::interface;
    std::string indent { "    " };
    void begin_block(const std::string& name) const override { 
        os << tab.back() << "* " << name << ":" << std::endl;
        tab.push_back(tab.back() + indent);
    }

    void end_block() const override {
        tab.pop_back();
    }

    void item(const std::string& name, const std::string& msg, const std::string& type) const override {
        os << tab.back() << "* " << name << ": " << msg << std::endl;
    }

    mutable std::vector<std::string> tab { "" };

    void begin_ordered() const override {
    }

    void end_ordered() const override {
    }
};


} // namespace formatter
} // namespace polysync::console

