#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ConfigReader.h"

namespace py = pybind11;

PYBIND11_MODULE(configreader_cpp, m) {
    m.doc() = "Python bindings for the C++ ConfigReader using pybind11";

    py::class_<ConfigReader>(m, "ConfigReader")
        .def(py::init<const std::string &>(), py::arg("filename"))
        .def("get_string", &ConfigReader::getString,
             py::arg("key"), py::arg("default") = std::string())
        .def("get_int", &ConfigReader::getInt,
             py::arg("key"), py::arg("default") = 0)
        .def("get_double", &ConfigReader::getDouble,
             py::arg("key"), py::arg("default") = 0.0)
        .def("get_float", &ConfigReader::getFloat,
             py::arg("key"), py::arg("default") = 0.0f)
        .def("get_int_vector", &ConfigReader::getIntVector, py::arg("key"));
}
