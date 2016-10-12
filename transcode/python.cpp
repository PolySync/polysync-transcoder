#include <polysync/transcode/reader.hpp>

#include <pybind11/pybind11.h>
#include <boost/filesystem.hpp>

namespace py = pybind11;
namespace fs = boost::filesystem;
namespace plog = polysync::plog;

PYBIND11_PLUGIN(polysync) {
    py::module m("polysync", "PolySync plog interface");

    py::class_<plog::reader>(m, "reader")
        // .def(py::init<const std::string&>())
        .def("__enter__", [](plog::reader& s){})
        // .def("__exit__", [](plog::reader&& s){ return std::move(s); })
        .def("__iter__", [](plog::reader& s) { 
                std::function<bool (plog::iterator)> filter = [](plog::iterator it) { return true; };
                    return py::make_iterator(s.begin(filter), s.end()); 
                },
                py::keep_alive<0, 1>() // keeps object alive while iterator exists!
                )
        ;

    py::class_<plog::log_record>(m, "log_record")
        .def_readonly("index", &plog::log_record::index)
        ;

    return m.ptr();
}


