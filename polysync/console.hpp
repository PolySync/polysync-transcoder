#pragma once

// Define console output styles.  This should be easy to extend to arbitrary
// formatting schemes if anyone ever cares to customize the look.  The nocolor
// style is important because the escape codes end up uglifying output devices
// that are not ansi-aware, like debugger displays, and a GUI tool would
// certainly want to control the look by itself. 

#include <string>
#include <vector>
#include <memory>
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

} // namespace console

namespace formatter {

using polysync::console::escape;

struct interface {
    virtual std::string header(const std::string& msg) const { return msg; };

    // Logging severity levels
    virtual std::string error(const std::string& msg) const { return msg; };
    virtual std::string warn(const std::string& msg) const { return msg; };
    virtual std::string info(const std::string& msg) const { return msg; };
    virtual std::string verbose(const std::string& msg) const { return msg; };
    virtual std::string debug(const std::string& msg) const { return msg; };
    virtual std::string channel(const std::string& msg) const { return msg; };

    // Datatypes
    virtual std::string fieldname(const std::string& msg) const { return msg; };
    virtual std::string type(const std::string& msg) const { return msg; };
    virtual std::string value(const std::string& msg) const { return msg; };

    // Document structure (like Markdown stuff)
    virtual std::string begin_block( const std::string& name, const std::string& meta = "" ) const = 0;
    virtual std::string end_block(const std::string& meta = "") const = 0;
    virtual std::string item( const std::string&, const std::string&, const std::string& = "" ) const = 0;
    virtual std::string begin_ordered(size_t idx, const std::string& name) const { 
        return std::to_string(idx) + ": " + name + " {"; 
    }
    virtual std::string end_ordered() const { return std::string(); }
};

struct fancy : interface {

    std::string indent { "    " };

    std::string header(const std::string& msg) const override { 
        return std::string(escape::bold) + escape::green + msg + escape::normal;  
    };

    std::string channel(const std::string& msg) const override { 
        return escape::green + msg + escape::normal; 
    };

    std::string fieldname(const std::string& msg) const override { 
        return escape::green + msg + escape::normal; 
    };

    std::string type(const std::string& msg) const override { 
        return escape::cyan + msg + escape::normal; 
    };

    std::string begin_block(const std::string& name, const std::string& meta) const override { 
        std::string result = tab.back() + escape::cyan + escape::bold + name + escape::normal;
        if (!meta.empty())
            result += " [" + meta + "]"; 
        result += std::string(escape::cyan) + escape::bold + " {\n" + escape::normal;
        tab.push_back(tab.back() + indent);
        return result;
    }

    std::string end_block(const std::string& meta) const override {
        tab.pop_back();
        return tab.back() + escape::cyan + escape::bold + "} " + meta + "\n" + escape::normal;
    }

    std::string item(const std::string& name, const std::string& value, const std::string& type) const override {
        std::string result = tab.back() + escape::green + name + ": " + escape::normal 
            + value + escape::normal;
        return result + "\n";
    }

    std::string begin_ordered(size_t rec, const std::string& name) const override {
        std::string result = tab.back() + std::to_string(rec) + ": " + name + " {\n";
        tab.push_back(tab.back() + indent);
        return result;
    }

    std::string end_ordered() const override {
        tab.pop_back();
        return tab.back() + "}\n";
    }

    std::string error(const std::string& msg) const override { return escape::red + msg + escape::normal; }
    std::string warn(const std::string& msg) const override { return escape::magenta + msg + escape::normal; }
    std::string info(const std::string& msg) const override { return escape::green + msg + escape::normal; }
    std::string verbose(const std::string& msg) const override { return escape::green + msg + escape::normal; }
    std::string debug(const std::string& msg) const override { return escape::white + msg + escape::normal; }

    mutable std::vector<std::string> tab { "" };
};

struct plain : interface {
    using interface::interface;

    std::string indent { "    " };

    std::string begin_block(const std::string& name, const std::string& meta) const override { 
        std::string result = tab.back() + name;
        if (!meta.empty())
            result +=  " [" + meta + "]"; 
        result += " {\n";
        tab.push_back(tab.back() + indent);
        return result;
    }

    std::string end_block(const std::string& meta) const override {
        tab.pop_back();
        return tab.back() + "} " + meta + "\n";
    }

    std::string item(const std::string& name, const std::string& value, const std::string& type) const override {
        std::string result = tab.back() + name + ": " + value;
        return result + "\n";
    }

    std::string begin_ordered(size_t rec, const std::string& name) const override {
        std::string result = tab.back() + std::to_string(rec) + ": " + name + " {\n";
        tab.push_back(tab.back() + indent);
        return result;
    }

    std::string end_ordered() const override {
        tab.pop_back();
        return tab.back() + "}\n";
    }

    mutable std::vector<std::string> tab { "" };
};

} // namespace formatter

extern std::shared_ptr<formatter::interface> format;

} // namespace polysync

