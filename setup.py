from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "configreader_cpp",
        ["config/configreader_bindings.cpp"],
        cxx_std=17,
    )
]

setup(
    name="configreader-cpp",
    version="0.1.0",
    description="Python bindings for the C++ ConfigReader",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
